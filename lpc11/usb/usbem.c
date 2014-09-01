/**
@file usbem.c
@brief Emulované rozhraní hw LPC1343 USB kontroleru.

Tento soubor, nahrazující @ref usbhw.c z firmware se snaží emulovat kontroler USB procesoru LPC1343 pomocí gadgetfs. Do vyšších vrstev firmware - software se snaží vůbec nezasahovat. Princip záleží v tom, že se zapíšou deskriptory zařízení (včetně endpointů) do /dev/gadget/dummy_udc, čímž se celý systém aktivuje. Pro každý endpoint je pak vytvořeno samostatné vlákno, data se zapisují / čtou do / ze souborů /dev/gadget/ep-X, X je jakési pojmenování, ne zcela jasné. Prostě to tak je, gadgetfs je tak postaven. Základ je převzat z příkladu <a href="www.linux-usb.org/gadget/usb.c">usb.c</a> pro driver v USER MODE, původní funkce usbhw.c jsou zachovány, byť by třeba nebyly funkční. Emulace tedy není úplná, obsluha událostí na EP0 je o něco zjednodušena, používá jen jediné vlákno pro čtení i zápis.

Přibližná struktura firmware je následující:
@dot
digraph g {
  graph [fontsize=10 labelloc="t" label="" splines=true overlap=false rankdir = "BT"];
  ratio = auto;
  "layer0" [ style = "filled, bold" penwidth = 2 fillcolor = "white" fontname = "Courier New"
  shape = "Mrecord" label =<<table border="0" cellborder="0" cellpadding="3" bgcolor="white">
  <tr><td bgcolor="yellow" align="center" colspan="2"><font color="blue">usbhw.c / usbem.c</font></td></tr>
  <tr><td align="left" port="r0">USB_IRQHandler</td></tr>
  <tr><td align="left" port="r1">USB_ReadEP</td></tr>
  <tr><td align="left" port="r2">USB_WriteEP</td></tr>
  <tr><td align="left" port="r3"> ... </td></tr></table>> ];
  "layer1" [ style = "filled, bold" penwidth = 1 fillcolor = "white" fontname = "Courier New"
  shape = "Mrecord" label =<<table border="0" cellborder="0" cellpadding="3" bgcolor="white">
  <tr><td bgcolor="yellow" align="center" colspan="2"><font color="black">usbcore.c</font></td></tr>
  <tr><td align="left" port="r0">USB_EndPoint0</td></tr>
  <tr><td align="left" port="r1">SetupPacket</td></tr>
  <tr><td align="left" port="r2">USB_DeviceStatus</td></tr>
  <tr><td align="left" port="r3">USB_DeviceAddress</td></tr>
  <tr><td align="left" port="r4">USB_Configuration</td></tr>
  <tr><td align="left" port="r5"> ... </td></tr></table>> ];
  "layer2" [ style = "filled, bold" penwidth = 1 fillcolor = "white" fontname = "Courier New"
  shape = "Mrecord" label =<<table border="0" cellborder="0" cellpadding="3" bgcolor="white">
  <tr><td bgcolor="yellow" align="center" colspan="2"><font color="black">usbdesc.c</font></td></tr>
  <tr><td align="left" port="r0">USB_DeviceDescriptor</td></tr>
  <tr><td align="left" port="r1">USB_ConfigDescriptor</td></tr>
  <tr><td align="left" port="r2">USB_StringDescriptor</td></tr></table>> ];
  "layer3" [ style = "filled, bold" penwidth = 1 fillcolor = "white" fontname = "Courier New"
  shape = "Mrecord" label =<<table border="0" cellborder="0" cellpadding="3" bgcolor="white">
  <tr><td bgcolor="yellow" align="center" colspan="2"><font color="black">usbuser.c</font></td></tr>
  <tr><td align="left" port="r0">USB_P_EP</td></tr>
  <tr><td align="left" port="r1">USB_EndPoint0</td></tr>
  <tr><td align="left" port="r2">USB_EndPoint1</td></tr>
  <tr><td align="left" port="r3">USB_EndPoint2</td></tr>
  <tr><td align="left" port="r4"> ... </td></tr></table>> ];
  "layer4" [ style = "filled, bold" penwidth = 1 fillcolor = "white" fontname = "Courier New"
  shape = "Mrecord" label =<<table border="0" cellborder="0" cellpadding="3" bgcolor="white">
  <tr><td bgcolor="yellow" align="center" colspan="2"><font color="black">mscuser.c</font></td></tr>
  <tr><td align="left" port="r0">MSC_BulkIn</td></tr>
  <tr><td align="left" port="r1">MSC_BulkOut</td></tr>
  <tr><td align="left" port="r2">MSC_GetCBW</td></tr>
  <tr><td align="left" port="r3">MSC_SetCBW</td></tr>
  <tr><td align="left" port="r4"> ... </td></tr></table>> ];
    layer3 -> layer1 [ penwidth = 2 ];
    layer1 -> layer0 [ penwidth = 2 ];
    layer1 -> layer2 [ penwidth = 2 ];
    layer0 -> layer3 [ penwidth = 2 ];
    layer4 -> layer0 [ penwidth = 2 ];
    layer3 -> layer4 [ penwidth = 2 ];
}

@enddot
@bug Zatím není zcela doladěno, pozastavení endpointů (stall) není dopsáno ale funguje to.
*/

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <asm/byteorder.h>
// Následující hlavičky můžou být problematické, lze použít ty z kernelu
#include <linux/types.h>
#include <linux/usb/gadgetfs.h>
#include <linux/usb/ch9.h>

#include "type.h"
#include "usb.h"
#include "usbcfg.h"
//#include "usb-desc.h"
#include "comp_desc.h"
#include "usbcore.h"
#include "usbuser.h"
//#include "mscuser.h"

#include "debug.h"
/// Takhle to asi vypadá v /dev/gadget:
static const char* gadgetDeviceNames[] = {
  "/dev/gadget/dummy_udc", "/dev/gadget/dummy_udc",   // EP0 control, pro kompatibilitu 2x
  "/dev/gadget/ep-a",      "/dev/gadget/ep-b",        // bulk/intr EPs
  "/dev/gadget/ep-c",      "/dev/gadget/ep-d",
  "/dev/gadget/ep-e",      "/dev/gadget/ep-f",
  "/dev/gadget/ep1out-bulk","/dev/gadget/ep1in-bulk",   // isochronous
  "/dev/gadget/ep2out-bulk","/dev/gadget/ep2in-bulk",   // isochronous
  NULL
};

#define CONFIG_VALUE  1
#define USB_BUFSIZE (1024)
#define MAX_LOGIC_EP  5
/**
Buffery pro jednotlivé EP
*/
typedef struct _USB_DataBuffer {
  uint8_t  buf[USB_BUFSIZE];      //!< místo pro data
  uint32_t len;                   //!< délka
  uint32_t ofs;                   //!< ofset
} USB_DataBuffer;
/// Fyzický endpoint - neukládá adresu EP, bude řízena indexem v poli
typedef struct _physicalEndpoint {
  USB_ENDPOINT_DESCRIPTOR*  pD;   //!< ukazatel na příslušný deskriptor
  char*                     name; //!< název v /dev/gadget (úplná cesta)
  int                       fd;   //!< a jemu příslušný filedeskriptor
  pthread_t                 thr;  //!< každý EP spouští thread
  USB_DataBuffer            buf;  //!< a má vlastní buffer
} physicalEndpoint_t;
/// Logický endpoint - kopíruje se vlastně struktura řadiče LPC1343
typedef struct _logicalEndpoint {
  int                 id;             //!< ID je v podstatě adresa,
                                      ///  odkazy pro thread by neměly být na zásobníku
  physicalEndpoint_t  out;            //!< jeden EP OUT
  physicalEndpoint_t  in;             //!< a jeden EP IN
  pthread_mutex_t     mut;            //!< mutex
  pthread_cond_t      flg;            //!< a podmínka zastavení threadu jsou společné
} logicalEndpoint_t;
/**
Datový kontejner pro tento soubor.
*/
typedef struct _USB_Container {
  int    verbose;                     //!< úroveň výpisu debug informací
  struct usb_device_descriptor          *device_desc;       //!< ukazatel na usb_device_descriptor
  struct usb_config_descriptor          *config;            //!< ukazatel na usb_config_descriptor
  
  enum usb_device_speed                 current_speed;      //!< rychlost zařízení (má být full)
  volatile int                          loop;               //!< podmínka hlavní smyčky
  volatile int                          connected;          //!< stav USB zařízení
  int                                   noack;
  logicalEndpoint_t                     ep[MAX_LOGIC_EP];   //!< 5 logických EP (adresa = index)
} USB_Container_t;

static USB_Container_t usbem;

/**************************************************************************************************/
extern uint32_t USB_ReadEP (uint32_t EPNum, uint8_t *pData);
uint32_t USB_ReadSetupEP   (uint32_t EPNum, uint8_t *pData) {
  return USB_ReadEP (EPNum, pData);
}

static void clearBuf (USB_DataBuffer* b) {
  b->len = 0;
  b->ofs = 0;
}

// forward declaration
static void stop_io  ();
static void close_fd (void *n_ptr);

/* you should be able to open and configure endpoints
 * whether or not the host is connected
 */
/**
Konfigurace Endpointu
@param n  index = adresa EP
@param dir směr, IN = 1. Rozliší fyzické EP.
@param label pomocná značka (funkce, z níž to bylo voláno)
@return filedeskriptor endpointu n, dir
*/
static int ep_config (int n, int dir, const char *label) {
  int   fd, status;
  char* name;
  physicalEndpoint_t*      pE;
  USB_ENDPOINT_DESCRIPTOR* pD;
  char  buf [USB_BUFSIZE];

  if (dir) pE = &usbem.ep[n].in;
  else     pE = &usbem.ep[n].out;
  name = pE->name;
  fd   = pE->fd;
  pD   = pE->pD;
  /* open and initialize with endpoint descriptor(s) */
  printf ("%s called from %s[%d]: %s=0x%02X\n",
          __func__, label, n, name, pD->bEndpointAddress);
  fd = open (name, O_RDWR);
  if (fd < 0) {
    status = -errno;
    fprintf (stderr, "%s open %s error %d (%s)\n",
             label, name, errno, strerror (errno));
    return status;
  }
  __u32* tmp = (__u32*) buf;
  /* one fs sets of config descriptors */
  *tmp = 1; /* tag for this format */
  memcpy (buf + 4, pD, USB_DT_ENDPOINT_SIZE);
  status = write (fd, buf, 4 + USB_DT_ENDPOINT_SIZE);
  
  if (status < 0) {
    status = -errno;
    fprintf (stderr, "Write %s config %s error %d (%s)\n",
             label, name, errno, strerror (errno));
    close (fd);
    return status;
  } else if (usbem.verbose) {
    unsigned long id;

    id = pthread_self ();
    fprintf (stderr, "%s start %ld fd %d\n", label, id, fd);
  }
  return fd;
}
/**
Vlákno, pro všechny IN stejné
@param param přenáší adresu logického EP. Rozlišení fyzických EP je dán směrem, zde IN
*/
static void *inp_thread_handler (void *param) {
  int  n = *(int *) param;
  int  fd, status, towrite;

  status = ep_config (n, 1, __FUNCTION__);
  if (status < 0)
    return NULL;
  fd = status;
  usbem.ep[n].in.fd = fd;

  pthread_cleanup_push (close_fd, &usbem.ep[n].in.fd);
  do {
    /* original LinuxThreads cancelation didn't work right
     * so test for it explicitly.
     */
    pthread_testcancel ();
    pthread_mutex_lock  (&usbem.ep[n].mut);                   // vstup do kritické sekce
    if (!usbem.ep[n].out.buf.len)    // TODO: tohle by taky asi mělo chodit po 64 bytech (a de fakto chodí)
      pthread_cond_wait (&usbem.ep[n].flg, &usbem.ep[n].mut); // počkej na signál (přijde z USB_WriteEP)
    towrite = usbem.ep[n].out.buf.len;                        // výstup z kritické sekce
    pthread_mutex_unlock(&usbem.ep[n].mut);

    status = write (fd, usbem.ep[n].out.buf.buf, towrite);
    if (status == towrite) {
      pthread_mutex_lock  (&usbem.ep[n].mut);           // další kritická sekce
      clearBuf (&usbem.ep[n].out.buf);
      if (usbem.verbose > 2) printf ("%d Bytes really written\n", status);
      EpHandlers[n].pEpn(EpHandlers[n].data, USB_EVT_IN);                          // tohle emuluje obsluhu přerušení od IN
      pthread_mutex_unlock(&usbem.ep[n].mut);           // konec kritické sekce
    }
  } while (status > 0);
  if (status == 0) {
    if (usbem.verbose)
      fprintf (stderr, "done %s\n", __FUNCTION__);
  } else if (usbem.verbose > 2 || errno != ESHUTDOWN)     // normal disconnect
    perror ("write");
  fflush (stdout);
  fflush (stderr);
  pthread_cleanup_pop (1);

  return NULL;
}
/**
Vlákno, společné pro všechny OUT.
@param param přenáší adresu logického EP. Rozlišení fyzických EP je dán směrem, zde OUT
*/
static void *out_thread_handler (void *param) {
  int  n = *(int *) param;
  int  status, toread, fd;

  status = ep_config (n, 0, __FUNCTION__);
  if (status < 0)
    return NULL;
  fd = status;
  usbem.ep[n].out.fd = fd;

  /* synchronous reads of endless streams of data */
  pthread_cleanup_push (close_fd, &usbem.ep[n].out.fd);
  do {
    /* original LinuxThreads cancelation didn't work right
     * so test for it explicitly.
     */
    pthread_testcancel ();
    errno  = 0;
    toread = 64;    // Odladěno: dlouhý požadavek (USB_BUFSIZE - usbem.rx.len) to zasekne
    // if (usbem.verbose > 2) printf ("Sink read enter %d\n", toread);
    status = read (fd, usbem.ep[n].in.buf.buf + usbem.ep[n].in.buf.len, toread);
    if (usbem.verbose > 2) printf ("Sink read %d\n", status);

    if (status < 0) {
      fprintf (stderr, "%s: read error status=%d, error=%d (%s)\n",
               __func__, status, errno, strerror(errno));
      break;
    }
    pthread_mutex_lock(&usbem.ep[n].mut);   // kritická sekce
    usbem.ep[n].in.buf.len += status;
    while (usbem.ep[n].in.buf.len)
      EpHandlers[n].pEpn (EpHandlers[n].data, USB_EVT_OUT);             // emulace obsluhy přerušení od OUT
    clearBuf (&usbem.ep[n].in.buf);
    pthread_mutex_unlock(&usbem.ep[n].mut); // konec kritické sekce
  } while (status > 0);
  if (status == 0) {
    if (usbem.verbose)
      fprintf (stderr, "done %s\n", __FUNCTION__);
  } else if (usbem.verbose > 2 || errno != ESHUTDOWN) /* normal disconnect */
    perror ("read");
  fflush (stdout);
  fflush (stderr);
  pthread_cleanup_pop (1);

  return NULL;
}
/**
Uzavření filedeskriptoru, jakýmsi způsobem testuje stav fronty endpointů (neznámo proč)
@param fd_ptr ukazatel na filedeskriptor
*/
static void close_fd (void *fd_ptr) {
  int status, fd;

  fd = * (int *) fd_ptr;
  * (int *)      fd_ptr = -1;

  printf ("\tClose_fd: %d\n", fd);
  /* test the FIFO ioctls (non-ep0 code paths) */
  if (pthread_self () != usbem.ep[0].in.thr) {
    status = ioctl (fd, GADGETFS_FIFO_STATUS);
    if (status < 0) {
      /* ENODEV reported after disconnect */
      if (errno != ENODEV && errno != EOPNOTSUPP)
        perror ("get fifo status");
    } else {
      fprintf (stderr, "fd %d, unclaimed = %d\n",
               fd, status);
      if (status) {
        status = ioctl (fd, GADGETFS_FIFO_FLUSH);
        if (status < 0)
          perror ("fifo flush");
      }
    }
  }

  if (close (fd) < 0)
    perror ("close");
}
/**
Pomocná funkce, přijme signál o ukončení a skončí hlavní smyčku ep0_thread.
@param sig  id signálu
@param info nepoužito
@param ptr  nepoužito
*/
static void signothing (int sig, siginfo_t *info, void *ptr) {
  // if (usbem.verbose)
    fprintf (stderr, "%s %d\n", __FUNCTION__, sig);
  usbem.loop = 0;
  // exit (-1);
}
/**
Obsluhuje události na endpointu EP0. Většina funkcionality přenesena do usbcore (funkce USB_EndPoint0). Pro značnou nepřehlednost není jisté, že je funkčnost úplná. Mechanizmus přenosu dat musel být přizpůsoben usbcore a není tedy úplně jednoduchý - data se zapíšou do in buferu, core s nimi provede nějaké operace a pokud něco zapsisuje, pak do buferu out. To je pak nutné zapsat do filedeskriptoru nad dummy_hcd.
@param setup kontrolní paket
*/
static void handle_control (struct usb_ctrlrequest *setup) {
  int  status, tmp;
  __u16  value, index, length;

  value  = __le16_to_cpu (setup->wValue);
  index  = __le16_to_cpu (setup->wIndex);
  length = __le16_to_cpu (setup->wLength);

  // if (usbem.verbose)
    fprintf (stderr, "SETUP %02x.%02x v%04x i%04x %d\n",
             setup->bRequestType, setup->bRequest, value, index, length);
  // obsluhujeme jen SETUP, jiná data nelze z gadgetfs vyčíst.
  tmp = sizeof (struct usb_ctrlrequest);
  clearBuf (&usbem.ep[0].in.buf);
  clearBuf (&usbem.ep[0].out.buf);
  memcpy(usbem.ep[0].in.buf.buf, setup, tmp);
  usbem.ep[0].in.buf.len += tmp;
  usbem.noack = 0;                  // ack pouze v SETUP stage
  EpHandlers[0].pEpn (EpHandlers[0].data, USB_EVT_SETUP);      // předhodíme to usbcore - parametr je event SETUP
  // Pokud přišla nějaká data navíc (DATA1), čtou se dost divně, ale jde to.
  if (length && (!(setup->bRequestType & 0x80))) {
    tmp = read (usbem.ep[0].in.fd, usbem.ep[0].in.buf.buf + usbem.ep[0].in.buf.ofs, length);
    usbem.ep[0].in.buf.len += length;
    if (tmp) {   // Opravdu, vrácená 0 je zde správně. Není jistota, že jsou data správně.
      fprintf(stderr, "%s: Read DATA1 error\n", __func__);
    }
    else {
      usbem.noack = 1;                // jdeme do DATA1 stage, ack už ne
      EpHandlers[0].pEpn (EpHandlers[0].data, USB_EVT_OUT);      // předhodíme to usbcore - parametr je event OUT
    }
  }
  if (usbem.ep[0].out.buf.len) {
    // TODO: při delších odpovědích by to asi chtělo volat něco jako USB_P_EP[0] (USB_EVT_IN)
    status = write (usbem.ep[0].out.fd, usbem.ep[0].out.buf.buf, usbem.ep[0].out.buf.len);
    if (usbem.verbose) printf("%s: %d bytes written\n", __func__, status);
    if (status == usbem.ep[0].out.buf.len) clearBuf (&usbem.ep[0].out.buf);
    else fprintf(stderr, "%s: write error\n", __func__);
  }
  return;
}
/**
Zastavení vstupně / výstupních operací stop vláken
*/
static void stop_io () {
  dbg();
  int n;
  for (n=1; n<MAX_LOGIC_EP; n++) {
    if (usbem.ep[n].in.pD) {        // Pouze aktivní endpoint
      if (usbem.verbose)
        printf ("Stop thread for EP%02X\n", usbem.ep[n].in.pD->bEndpointAddress);
      if (!pthread_equal (usbem.ep[n].in.thr, usbem.ep[0].in.thr)) {  // ale ne EP0
        pthread_cancel (usbem.ep[n].in.thr);
        if (pthread_join (usbem.ep[n].in.thr, 0) != 0)
          perror ("can't join source thread");
        usbem.ep[n].in.thr = usbem.ep[0].in.thr;
      }
      usbem.ep[n].in.pD = NULL;     // bude dále neaktivní
    }
    if (usbem.ep[n].out.pD) {
      if (usbem.verbose)
        printf ("Stop thread for EP%02X\n", usbem.ep[n].out.pD->bEndpointAddress);
      if (!pthread_equal (usbem.ep[n].out.thr, usbem.ep[0].out.thr)) {
        pthread_cancel (usbem.ep[n].out.thr);
        if (pthread_join (usbem.ep[n].out.thr, 0) != 0)
          perror ("can't join sink thread");
        usbem.ep[n].out.thr = usbem.ep[0].out.thr;
      }
      usbem.ep[n].in.pD = NULL;
    }
  }
}
/// Nastavení signálů pro ukončení programu, obsluhuje signothing
static void signals_set (void) {
  struct sigaction action;
  action.sa_sigaction = signothing;
  sigfillset (&action.sa_mask);
  action.sa_flags = SA_SIGINFO;
  if (sigaction (SIGINT,  &action, NULL) < 0) {
    perror ("SIGINT");
    return;
  }
  if (sigaction (SIGQUIT, &action, NULL) < 0) {
    perror ("SIGQUIT");
    return;
  }
}
#define NEVENT  5
/**
Toto zde je další vlákno. Neobluhuje všechy události EP0, mechanizmus je přečti událost a pokud to vygeneruje nějaká data pro zápis, zapiš. Obslouží tedy všechny pakety kratší než 64 bytů. To může způsobit problémy při přenosu delších string deskriptorů. Nicméně to nějak chodí.
@param param ukazatel na int, obsahující 0
*/
static void *ep0_thread (void *param) {
  int    fd, i, n = * (int*) param;     // n = 0
  pthread_t id;

  id = pthread_self ();
  for (i=0; i<MAX_LOGIC_EP; i++) {      // pro všechny EP vč. tohoto (0) nastav thread ID
    usbem.ep[i].out.thr = id;
    usbem.ep[i].in.thr  = id;
  }
  fd = usbem.ep[n].in.fd;
  
  pthread_cleanup_push (close_fd, &usbem.ep[n].in.fd);


  usbem.loop      = 1;
  usbem.connected = 0;
  /* event loop */
  while (usbem.loop) {
    int    tmp;
    struct usb_gadgetfs_event event [NEVENT];
    int    i, nevent;

    tmp = read (fd, &event, sizeof event[0]);
    // print("nacteno: %d\n", tmp);
    if (tmp < 0) {
      perror ("ep0 read after poll\n");
      break;
    }
    nevent = tmp / sizeof event [0];
    if (nevent != 1 && usbem.verbose)
      fprintf (stderr, "read %d ep0 events\n",
               nevent);

    for (i = 0; i < nevent; i++) {
      switch (event [i].type) {
      case GADGETFS_NOP:
        if (usbem.verbose)
          fprintf (stderr, "NOP\n");
        break;
      case GADGETFS_CONNECT:
        usbem.connected = 1;
        usbem.current_speed = event [i].u.speed;
        // if (usbem.verbose)
          fprintf (stderr, "CONNECT %d\n", event [i].u.speed);
        break;
      case GADGETFS_SETUP:
        usbem.connected   = 1;
        USB_Configuration = 1;
        handle_control (&event [i].u.setup);
        break;
      case GADGETFS_DISCONNECT:
        usbem.connected = 0;
        usbem.current_speed = USB_SPEED_UNKNOWN;
        // if (usbem.verbose)
          fprintf (stderr, "DISCONNECT\n");
        stop_io ();
        break;
      case GADGETFS_SUSPEND:
        // usbem.connected = 1;
        // if (usbem.verbose)
          fprintf (stderr, "SUSPEND\n");
        break;
      default:
        fprintf (stderr, "* unhandled event %d\n", event [i].type);
      }
    }
    continue;
  }
  // if (usbem.verbose)
    fprintf (stderr, "%s done\n", __func__);
  fflush (stdout);

  pthread_cleanup_pop (1);
  return NULL;
}

/**
Kontrola gadgetfs zařízení.
@return 0 při úspěchu
*/

static int autoconfig () {
  struct stat statb;

  // dummy_hcd, full speed
  if (stat (usbem.ep[0].in.name, &statb) == 0) {
  } else {
    usbem.ep[0].in.name  = NULL;
    usbem.ep[0].out.name = NULL;
    return -ENODEV;
  }
  return 0;
}
/**
Do buferu uloží ve správném pořadí jednotlivé deskriptory zařízení, včetně enpoint descriptorů. Vlastně ukládá celý config deskriptor, čímž se to zjednodušilo.
@param cp vstupní buffer
@return
*/

static char *build_config (char *cp) {
  int len = __cpu_to_le16 (usbem.config->wTotalLength);
  /*
  int ii;
  uint8_t* pi = (uint8_t*) usbem.config;
  for (ii=0; ii<len; ii++) printf ("<%02X>", pi[ii]);
  printf ("\nbuild_config, len=0x%X\n", len);
  */
  memcpy (cp, usbem.config, len);
  cp += len;
  return cp;
}

/*
 *  USB Initialize Function
 *   Called by the User to initialize USB
 *    Return Value:    None
 */
/**
Inicializace systému. Původní funkce, tělo je však nahrazeno emulací. Jen základní nastavení proměnných.
*/
void USB_Init (void) {
  int n = 0;
  // počáteční nastavení dat.
  for (n=0; n<MAX_LOGIC_EP; n++) {
    usbem.ep[n].out.fd   = -1;
    usbem.ep[n].out.name = (char*)gadgetDeviceNames[2*n];
    usbem.ep[n].out.pD   = NULL;
    usbem.ep[n].out.thr  = 0;
    
    usbem.ep[n].in.fd    = -1;
    usbem.ep[n].in.name  = (char*)gadgetDeviceNames[2*n+1];
    usbem.ep[n].in.pD    = NULL;
    usbem.ep[n].in.thr   = 0;
    
    usbem.ep[n].id       = n;
    clearBuf(&usbem.ep[n].out.buf);
    clearBuf(&usbem.ep[n].in.buf);
    
    pthread_mutex_init(&usbem.ep[n].mut, NULL);
    pthread_cond_init (&usbem.ep[n].flg, NULL);
  }
  usbem.device_desc = (struct usb_device_descriptor*) USB_DeviceDescriptor;
  usbem.config      = (struct usb_config_descriptor*) pConfigDescriptor;
  
  usbem.verbose = 0;
}
/*
 *  USB Connect Function
 *   Called by the User to Connect/Disconnect USB
 *    Parameters:      con:   Connect/Disconnect
 *    Return Value:    None
 */
/**
Otevře dummy_hcd, zapíše deskriptory a spustí obsluhu EP0.
@param con 1 = connect, inicializace, 0 nedělá nic.
*/
void USB_Connect (uint32_t con) {
  dbg();
  char  buf [4096], *cp = buf;
  int  fd;
  int  status;
  
  if (!con) return;
  status = autoconfig ();
  if (status < 0) {
    fprintf (stderr, "?? don't recognize %s device bulk\n", usbem.ep[0].in.name);
    return;
  }

  fd = open (usbem.ep[0].in.name, O_RDWR);
  if (fd < 0) {
    perror (usbem.ep[0].in.name);
    return;
  }

  * (__u32 *) cp = 0; /* tag for this format */
  cp += 4;

  /* write full speed config */
  cp = build_config (cp);
  /* and device descriptor at the end */
  memcpy (cp, usbem.device_desc, sizeof (struct usb_device_descriptor));
  cp += sizeof (struct usb_device_descriptor);
  /*
  int ii;
  char* px = (char*) usbem.device_desc;
  for (ii=0; ii<sizeof (struct usb_device_descriptor); ii++) printf ("<%02X>", px[ii]);
  putchar ('\n');
  */
  int xlen = cp - buf;
  //printf ("xlen = %d\n", xlen);
  status = write (fd, buf, xlen);
  if (status < 0) {
    perror ("write dev descriptors");
    close (fd);
    return;
  } else if (status != (cp - buf)) {
#if __SIZEOF_POINTER__ == 8
    fprintf (stderr, "dev init, wrote %d expected %ld\n", status, cp - buf);
#else
    fprintf (stderr, "dev init, wrote %d expected %d\n", status, cp - buf);
#endif
    close (fd);
    return;
  }
  int n = 0;
  usbem.ep[n].in.fd  = fd;    // pro kompatibitilu do obou, stejně
  usbem.ep[n].out.fd = fd;
  print (" dev=%s, fd=%d\n", usbem.ep[0].in.name, fd);
  fprintf (stderr, "%s ep0 configured\n", usbem.ep[n].in.name);
  fflush  (stderr);
  if (pthread_create (&usbem.ep[n].in.thr, NULL,
                      ep0_thread, (void *) &usbem.ep[n].id) != 0) {
    perror ("can't create source thread");
  }
  signals_set();    // Nastaví signály tak, aby to šlo ukončit Ctrl-C
  return;
}
/// Uklidí po sobě - stopne vlákna a tím zavře soubory
void  USB_Cleanup   (void) {
  dbg();
  int n = 0;
  if (usbem.connected)  stop_io ();
  pthread_cancel (usbem.ep[n].in.thr);
  if (pthread_join (usbem.ep[n].in.thr, 0) != 0)
    perror ("can't join source thread");
}


/*
 *    USB and IO Clock configuration only.
 *    The same as call PeriClkIOInit(IOCON_USB);
 *    The purpose is to reduce the code space for
 *    overall USB project and reserve code space for
 *    USB debugging.
 *    Parameters:      None
 *    Return Value:    None
 */
/// Tohle není v emulaci potřeba.
void USBIOClkConfig (void) {
  dbg();
  // zde nedelej nic, jen pro kompatibitilu
  return;
}
/*
 *  USB Reset Function
 *   Called automatically on USB Reset
 *    Return Value:    None
 */
/// Funkce, která nic nedělá ani v usbhw.
void USB_Reset (void) {
  dbg();
}


/*
 *  USB Suspend Function
 *   Called automatically on USB Suspend
 *    Return Value:    None
 */

/// Funkce, která nic nedělá ani v usbhw.
void USB_Suspend (void) {
  dbg();
  /* Performed by Hardware */
}


/*
 *  USB Resume Function
 *   Called automatically on USB Resume
 *    Return Value:    None
 */

/// Funkce, která nic nedělá ani v usbhw.
void USB_Resume (void) {
  dbg();
  /* Performed by Hardware */
}


/*
 *  USB Remote Wakeup Function
 *   Called automatically on USB Remote Wakeup
 *    Return Value:    None
 */

/// Funkce, která nic nedělá ani v usbhw.
void USB_WakeUp (void) {
  dbg();
}


/*
 *  USB Remote Wakeup Configuration Function
 *    Parameters:      cfg:   Enable/Disable
 *    Return Value:    None
 */

/// Funkce, která nic nedělá ani v usbhw.
void USB_WakeUpCfg (uint32_t cfg) {
  print("%d\n", cfg);
  cfg = cfg;  /* Not needed */
}


/*
 *  USB Set Address Function
 *    Parameters:      adr:   USB Address
 *    Return Value:    None
 */

/// Funkce, která nic nedělá ani v usbhw.
void USB_SetAddress (uint32_t adr) {
  print("%d\n", adr);
}


/*
 *  USB Configure Function
 *    Parameters:      cfg:   Configure/Deconfigure
 *    Return Value:    None
 */
/**
Funkcionalita nastavení konfigurace byla přenesena právě sem. U emulátoru spouští I/O operace.
@param cfg 1 = spustit, 0 = zastavit
*/
void USB_Configure (uint32_t cfg) {
  
  if (usbem.verbose) print("%d\n", cfg);
  switch (cfg) {
    case 0:
      stop_io ();
    case CONFIG_VALUE:    // Přeneseno do USB_ConfigEP
      break;
    default:
      /* kernel bug -- "can't happen" */
      fprintf (stderr, "? illegal config %d\n", cfg);
      break;
  }
}


/*
 *  Configure USB Endpoint according to Descriptor
 *    Parameters:      pEPD:  Pointer to Endpoint Descriptor
 *    Return Value:    None
 */
/// To je voláno z usbcore.
void USB_ConfigEP (USB_ENDPOINT_DESCRIPTOR *pEPD) {
  uint8_t Adr = pEPD->bEndpointAddress;
  uint8_t   n = Adr & 0x0F;
  
  print ("0x%02X\n", pEPD->bEndpointAddress);
  if (Adr & 0x80) {
    if (usbem.ep[n].in.pD) {    // EP already configured
      return;
    }
    usbem.ep[n].in.pD = pEPD;
    if (pthread_create (&usbem.ep[n].in.thr, NULL,
                        inp_thread_handler, (void *) &usbem.ep[n].id) != 0) {
      perror ("can't create source thread");
      return;
    }
  }
  else {
    if (usbem.ep[n].out.pD) {
      return;
    }
    usbem.ep[n].out.pD = pEPD;
    if (pthread_create (&usbem.ep[n].out.thr, NULL,
                        out_thread_handler, (void *) &usbem.ep[n].id) != 0) {
      perror ("can't create sink thread");
      return;
    }
  }
  return;
}


/*
 *  Set Direction for USB Control Endpoint
 *    Parameters:      dir:   Out (dir == 0), In (dir <> 0)
 *    Return Value:    None
 */
/// To není celkem potřeba
void USB_DirCtrlEP (uint32_t dir) {
  if (usbem.verbose)
    print("%d\n", dir);
}


/*
 *  Enable USB Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */
/// V zásadě to už vykonala fce USB_ConfigEP.
void USB_EnableEP (uint32_t EPNum) {
  if (usbem.verbose)
    print("%02X\n", EPNum);
}


/*
 *  Disable USB Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */
/// Nahrazeno stop_io.
void USB_DisableEP (uint32_t EPNum) {
  // if (usbem.verbose)
    print("%02X\n", EPNum);
}


/*
 *  Reset USB Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */

/// Funkce, která nic nedělá ani v usbhw.
void USB_ResetEP (uint32_t EPNum) {
  if (usbem.verbose)
    print("%02X\n", EPNum);
}


/*
 *  Set Stall for USB Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
#include <unistd.h>
 */
/**
Tohle zatím nefunguje, ač by to bylo potřeba. V zásadě není třeba nic dělat, pokud je EP IN. Prostě dál neposíláme data. U OUT se zatím problém nevyskytl. Mělo by to fungovat tak, že se do EP zapíše jakoby v opačném směru. Ale pokud udělám read na EP IN, zablokuje se to (logicky). Write na EP OUT by měl být bez problémů, ale zatím jsem to napotřeboval, tak to nic nedělá.
@param EPNum adresa endpointu
*/
void USB_SetStallEP (uint32_t EPNum) {
  print("%02X\n", EPNum);
  /*
  uint8_t n;
  if (EPNum & 0x80) return;
  n = EPNum;
  n = read(usbem.ep[n].out.fd, &n, 0);
  if (n) fprintf(stderr, "err=%s\n", strerror(errno));
  */
}


/*
 *  Clear Stall for USB Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */
/// Na tohle by měl být ioctl. Zatím nebylo potřeba.
void USB_ClrStallEP (uint32_t EPNum) {
  print("%02X\n", EPNum);
}


/*
 *  Clear USB Endpoint Buffer
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */
/// To taky zatím není třeba.
void USB_ClearEPBuf (uint32_t EPNum) {
  print("%02X\n", EPNum);
}


/*
 *  Read USB Endpoint Data
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *                     pData: Pointer to Data Buffer
 *    Return Value:    Number of bytes read
 */
/**
Fakticky předává data do vlákna typu OUT. Tento poněkud těžkopádný způsob je použit, protože se z thread handleru musí volat něco, co se chová jako obsluha přerušení na EP. A právě z vnitřku tohoto přerušení je pak volána tato funkce.
@param EPNum adresa endpointu
@param pData pointer na data
@return počet vrácených bytů
*/
uint32_t USB_ReadEP (uint32_t EPNum, uint8_t *pData) {
  uint32_t n, len = 0;
  
  if (usbem.verbose > 2) print(" (%02X)\n", EPNum);
  if (EPNum & 0x80) fprintf(stderr, "%s: Dir err\n", __func__);
  n = EPNum & 0x0F;
  if (usbem.ep[n].in.buf.len > 64)  len = 64;
  else                              len = usbem.ep[n].in.buf.len;
  memcpy (pData, usbem.ep[n].in.buf.buf + usbem.ep[n].in.buf.ofs, len);
  usbem.ep[n].in.buf.len -= len;
  usbem.ep[n].in.buf.ofs += len;
  return len;
}
/*
 *  Write USB Endpoint Data
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *                     pData: Pointer to Data Buffer
 *                     cnt:   Number of bytes to write
 *    Return Value:    Number of bytes written
 */

/**
Fakticky předává data z vlákna typu IN. Tento poněkud těžkopádný způsob je použit, protože se z thread handleru musí volat něco, co se chová jako obsluha přerušení na EP. A právě z vnitřku tohoto přerušení je pak volána tato funkce.
@param EPNum adresa endpointu
@param pData pointer na data
@param cnt   počet zapisovaných bytů
@return počet vrácených bytů
*/
uint32_t USB_WriteEP (uint32_t EPNum, uint8_t *pData, uint32_t cnt) {
  uint32_t n, len = cnt;
  
  if (usbem.verbose > 2) print ("(0x%02X, %d)\n", EPNum, cnt);
  if ((EPNum == 0x80) && (cnt == 0)) {    // potvrzení na EP0 (čtení naprázdno)
    if (usbem.noack) return 0;            // pouze pokud je potřeba
    // ack ...
    len = read (usbem.ep[0].in.fd, &len, 0);
    if (len)
      perror ("ack SET_CONFIGURATION");
    return 0;
  }
  if (!cnt) return 0;
  if ((EPNum & 0x80) == 0) fprintf(stderr, "%s: Dir err\n", __func__);
  n =  EPNum & 0x0F;
  memcpy (usbem.ep[n].out.buf.buf + usbem.ep[n].out.buf.ofs, pData, len);
  usbem.ep[n].out.buf.len += len;
  usbem.ep[n].out.buf.ofs += len;
  pthread_cond_signal (&usbem.ep[n].flg);       // probudí inp thread (zápis do EP)
  if (usbem.ep[n].out.buf.ofs > USB_BUFSIZE)
    fprintf(stderr, "%s buffer owerflow :%d\n", __func__, usbem.ep[n].out.buf.ofs);
  return len;
}

/*
 *  Get USB Last Frame Number
 *    Parameters:      None
 *    Return Value:    Frame Number
 */
/// ???
uint32_t USB_GetFrame (void) {
  dbg();
  return 0;
}


/*
 *  USB Interrupt Service Routine
 */
/**
Obsluha přerušení. Zde není obsluhováno. V HW se v těle tt. funkce volají jednotlivé funkce USB_P_EP[n](USB_EVT_XXX). Zde jsou volány v omezené míře jednotlivými thread handlery. Funkčnost tedy nemusí být úplná.
*/
void USB_IRQHandler (void) {
  dbg();
  return;
}
/// Zatím o sobě jen říká, že probíhá. Je to však nutné, aby šel software korektně ukončit Ctrl-C.
int  USB_MainLoop   (void) {
  // static int n = 0;
  // print (" %d\n", n++);
  usleep (10000);
  if (usbem.loop) return 0;
  else            return 1;
}
