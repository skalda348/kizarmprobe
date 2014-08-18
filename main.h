#include "swdp.h"
#include "socket.h"
#include "gdbpacket.h"
#include "gdbserver.h"
#include "stm32f1.h"
#include "stm32f4.h"
#include "lpc11xx.h"

/**
 * @mainpage Kizarm Probe.
 * Tento projekt je inspirován Blackmagic Probe pro STM32Fxx. Pro procesory NXP něco takového trochu
 * chybí, tak se to pokusíme napravit.
 * 
 * @section sectA Jak to vlastně funguje.
 * Hojně používaným prostředkem pro ladění jednočipů je OpenOCD. Je to proto, že podporuje velkou
 * spoustu debug adaptérů (jako je např. ST-Link) a umožňuje ladit širokou škálu cílových, target procesorů.
 * Je to však poněkud nešikovné - musíme to pustit jako server a k němu se z jedné strany připojí laděný
 * procesor a z druhé gdb. Blackmagic to řeší úsporněji - ten server běží přímo v hardware debug adaptéru.
 * Což poněkud omezuje šířku záběru, běží to s velmi omezenými prostředky, takže nelze chtít zázraky.
 * Nicméně je to velmi povedený kousek firmware. Je sice zaměřen především na procesory STM32, na němž je
 * postaven, ladit se však dají i jiné. Předmětem mého zájmu bylo, zda by nešlo tento firmware trochu
 * upravit, aby běžel na NXP procesoru LPC11U24. Ten má totiž USB stack postaven jako knihovnu, vypálenou
 * přímo do ROM, takže se s tím není nutné tak podrobně zaobírat.
 * 
 * @section sectB Co vše se tedy změnilo.
 * Skoro všechno. Aby se to vešlo do omezené flash i ram, bylo potřeba spoustu věcí vyházet. Chtěl jsem to
 * především na ladění Cortex-M0 (a možná i vyšších, ale stále ARM), takže jsem vyhodil JTAG. Cortexy
 * se dají ladit po SWD, takže potřebujeme jen 2 dráty a zem. To je sice trochu omezující (chtělo by to
 * ještě reset), ale dá se s tím žít. Větší škoda bylo vyhodit virtuální sériový port, ten je pro ladění
 * o něco potřebnější. Ale ten by se asi dal do tohoto projektu přidat. Nejsou lidi.
 * No a protože SWD, tak není ani multitarget. Není potřeba.
 * 
 * Nakonec to celé dopadlo tak, že jsem to přepsal do C++. Některé kusy kódu byly v čistém C docela dost
 * zamotané a tak jsem je ani já úplně nepochopil. Ona je to docela jednoduchá skládačka z objektů,
 * ale když píšeme objekty v C a dělá na tom více  lidí, vznikne dost velký guláš. C++ je na tom o poznání lépe
 * a velikost kódu neroste nijak dramaticky.
 * 
 * Dále bylo potřeba trochu pozměnit přístup k handshakingu na USB. NXP driver to dělá tak, že při příjmu
 * setrvá v přerušení dokud nejsou data zpracována, což korektně pozastaví endpoint. Původní metoda zpracovávala
 * data v hlavní smyčce a případné pozastavení si uměla vynutit. To se mi na NXP nepovedlo. Takže zůstáváme
 * v přerušení, kde se generuje i odpověď na gdb paket. V main() je jediná metoda, která zjišťuje, zda
 * target procesor běží. Ono to zase tak moc nevadí, jen je nutné mít dost dlouhou výstupní frontu, aby se do ní
 * vše vešlo. Samotné gdb je inteligentní - na začátku se zeptá jak dlouhé pakety umí připojený adaptér
 * zpracovat a pak používá maximálně tuto délku. Problémem jsou pakety, kterými odpovídá uživatelský
 * monitor (např. příkaz monitor help). Takže kvůli této blbosti musí být výstupní fronta dlouhá.
 * Zde jsou samotné gdb pakety zkráceny na 256 bytů, což zdá se stačí.
 * 
 * @section sectC Paměť.
 * Takže se dostáváme k rozložení RAM. Lze to celé natěsnat do 8kB. Samotné statické třídy, ze kterých
 * je to postaveno moc místa nezabírají. Ta výstupní fronta má 1kB, což je pro gdb pakety dostatečná
 * rezerva a když přeteče zase tak moc se neděje - hláška se nevypíše celá, server však běží dál.
 * Dále je v .bss heap o velikosti 2kB, kde je celý target a některá data různých těch sprintf, u kterých
 * je těžké předem určit délku. Od 4kB je oblast USB driverů o níž toho víme dost málo, ale nebude
 * zabírat víc jak 2kB - to je délka USB oblasti některých procesorů. Předpokládá se, že tato oblast
 * k dispozizi není - pokud by byla, je možné přesunout drivery tam (./lpc11/usbclass.h, metoda init()).
 * Poslední 2kB zabírá stack, měl by být dost velký, protože některé metody mají na stacku dost
 * velká pole dat.
 * 
 * @section sectD Struktura adresářů, firmware a ladění.
 * -# kořen, kde je i Makefile, obsahuje jen jednoduchý main.cpp a main.h.
 * -# ./src Společné třídy pro firmware i ladění.
 * -# ./inc Společné hlavičky pro firmware i ladění.
 * -# ./lpc11 Obsahuje třídy a hlavičky jen pro firmware.
 * -# ./i386 Obsahuje třídy a hlavičky pro ladění na PC.
 * -# ./lib Obsahuje pomocné utility pro firmware, včetně zdrojáků a ld skriptu.
 * -# ./dbg Zde je pomocný firmware pro ladění na PC.
 * -# ./cmsis je CMSIS.
 * 
 * Struktura programu vypadá na první pohled složitě, ale je dost prostá. Základem je třída BaseLayer,
 * pomocí níž jsou propojeny tyto části:
 * Swdp - GdbServer - GdbPacket - Socket.
 * -# Swdp zajišťuje fyzický přístup na SWD piny. Je to jeden konec řetězu.
 * -# GdbServer je jádrem celého problému.
 * -# GdbPacket je mezivrstva obsluhující jednotlivé pakety gdb. 
 * -# Socket je poněkud nešťastně nazvaný druhý konec řetězce, protože v PC je to opravdu síťový socket,
 * ve vlastním firmware by se to mělo spíš jmenovat CDC_class, protože to opravdu dělá virtuální
 * sériový port.
 * 
 * Jádrem je jak bylo uvedeno GdbServer, bude proto popsán samostatně. Obsahuje především vlastní
 * Target, který je zde vytvářen dynamicky metodou Scan() - gdb příkaz "monitor scan".
 * 
 * Takže pokud chceme ladit firmware na PC, což je daleko příjemnější, spustíme Makefile v ./dbg a vytvořený
 * firmware.bin nalejeme do LPC11U24 (nebo 34 podle libosti - konečný cíl by měl mít 32kB flash a 8kB ram).
 * V kořenovém Makefile odkomentujeme include pc.inc, zakomentujeme #include lp.inc a spustíme.
 * Výsledný firmware.elf jde pod Linuxem spustit jako gdb server na portu 3333. Tedy pokud máme připojen
 * na USB ten připravený procesor jako /dev/ttyACM0. Takže pokud máme i připojené piny (viz ./lpc11/swdp.h)
 * na laděný target, funguje to podobně jako OpenOCD. Takto lze ladit vše, co je v ./src (a ./inc).
 * Když máme odladěno, změníme zpět komentáře v Makefile a vytvoříme výsledný firmware.bin, který už
 * má v sobě gdb server.
 * Kontrolní sumy vektorů, pro LPC11... dost důležité by měly sedět, takže stačí binárky opravdu jen
 * prostě zkopírovat.
 * 
  */