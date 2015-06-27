#include "swdp.h"
#include "cdclass.h"
#include "gdbpacket.h"
#include "gdbserver.h"
#include "stm32f1.h"
#include "stm32f4.h"
#include "lpc11xx.h"

#ifdef SERIAL
#include "usart1.h"
#include "mirror.h"
#endif
/**
 * @mainpage Kizarm Probe.
 * Tento projekt je inspirován <a href="http://www.blacksphere.co.nz/main/blackmagic">Blackmagic Probe</a>
 * pro STM32Fxx. Pro procesory NXP něco takového trochu
 * chybí, tak se to pokusíme napravit. Měl by tak vzniknout jednoduchý a levný prostředek pro ladění
 * pomocí SWD založený na čipu LPC11U24 pro řadu LPC11Xxx ale nejen pro ni. Další targety bude možné
 * podle libosti přidávat, zdroj je zcela otevřen pro pokusy.
 * 
 * @author
 * -# Copyright (C) 2009 Keil - An ARM Company.
 *    \n USB stack pro NXP procesory
 * -# Copyright (C) 2011  Black Sphere Technologies Ltd.
 *    \n Gareth McMullin <gareth@blacksphere.co.nz>, gdb server a ARM utility.
 * -# Copyright (C) 2014 Miroslav Mráz <mrazik@volny.cz>
 *    \n Postaveno do tříd C++, upraveno pro GNU toolchain a slepeno dohromady.
 * 
 * @section sectLic Licence.
 * <a href="http://www.gnugpl.cz/">Obecná veřejná licence GNU (GNU GPL), 3. verze</a> \n
 * Tento program je svobodný software: můžete jej šířit a upravovat podle ustanovení Obecné veřejné licence GNU
 * (GNU General Public Licence), vydávané Free Software Foundation a to podle 3. verze této Licence.
 * 
 * <b>Tento program je rozšiřován v naději, že bude užitečný, avšak BEZ JAKÉKOLIV ZÁRUKY.
 * Neposkytují se ani odvozené záruky PRODEJNOSTI anebo VHODNOSTI PRO URČITÝ ÚČEL.
 * Další podrobnosti hledejte v Obecné veřejné licenci GNU.</b>
 * 
 * @section sectHw Hardware.
 * Bylo již publikováno <a href="http://mcu.cz/comment.php?comment.news.3515">zde</a>.
 * Pinout je trochu podivný (viz. ./lpc11/config.h), takže SWCLK je zde AD0 a SWDIO je AD1.
 * Sériový port, pokud je použit používá normálně piny RXD a TXD.
 * 
 * @section sectA Jak to vlastně funguje.
 * Hojně používaným prostředkem pro ladění jednočipů je
 * <a href="http://openocd.sourceforge.net/">OpenOCD</a>. Je to proto, že podporuje velkou
 * spoustu debug adaptérů (jako je např. ST-Link) a umožňuje ladit širokou škálu cílových, target procesorů.
 * Je to však poněkud nešikovné - musíme to pustit jako server a k němu se z jedné strany připojí laděný
 * procesor a z druhé gdb. <a href="http://www.blacksphere.co.nz/main/blackmagic">Blackmagic</a>
 * to řeší úsporněji - ten server běží přímo v hardware debug adaptéru.
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
 * o něco potřebnější. Ten sice jde do projektu přidat, ale je s tím víc problémů než užitku.
 * No a protože SWD, tak není ani multitarget. Není potřeba. Není potřeba ani DFU, NXP to řeší pomocí MSC (disk),
 * což je sice efektnější, ale má svá úskalí (zde snad vyřešena).
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
 * Dále je v .bss heap o velikosti 3kB, kde je celý target a některá data různých těch sprintf, u kterých
 * je těžké předem určit délku. A i ta výstupní fronta. Od 4kB je oblast USB driverů o níž toho víme
 * dost málo, ale nebude zabírat víc jak 2kB - to je délka USB oblasti některých procesorů. Předpokládá se,
 * že tato oblast k dispozizi není - pokud by byla, je možné přesunout drivery tam
 * (./lpc11/rom/usbclass.h, metoda init(), případně pokud nepoužijeme ROM drivery ./lpc11/usb/usbhw.h).
 * Poslední 2kB zabírá stack, měl by být dost velký, protože některé metody mají na stacku dost
 * velká pole dat.
 * 
 * @section sectDD Struktura adresářů, firmware a ladění.
 * -# kořen, kde je i Makefile, obsahuje jen komentář v main.h.
 * -# ./src Společné třídy pro firmware i ladění.
 * -# ./inc Společné hlavičky pro firmware i ladění.
 * -# ./lpc11 Obsahuje třídy a hlavičky jen pro firmware.
 *      -# rom Soubory pro použití ROM driverů USB.
 *      -# usb Soubory pro vlastní driver USB. NXP má na webu zdrojáky, ale je to psáno pro Keil
 *              překladač a bylo nutno to trochu upravit. Podle toho, jak to bylo původně napsáno
 *              bych tomu moc nevěřil. Nicméně to celkem chodí.
 * -# ./i386 Obsahuje třídy a hlavičky pro ladění na PC pod OS Linux.
 * -# ./lib Obsahuje pomocné utility pro firmware, včetně zdrojáků a ld skriptu.
 * -# ./dbg Zde je pomocný firmware pro ladění na PC.
 * -# ./com Zde je samostatný převodník USB <-> USART
 * -# ./cmsis je CMSIS.
 * -# ./product obsahuje 2 binární soubory pro LPC11U24,34 nejméně 32KB flash / 8KB ram.
 *      -# serial.bin - kompletní probe se sériovým portem, používá ty Keil drivery. Je vcelku
 *         použitelné (tedy pod Linuxem), ale sériový port má své mouchy. Prostě je to moc věcí
 *         najednou.
 *      -# probe.bin používá ROM drivery, nemá sériový port a tedy je to spolehlivější.
 *         Pro většinu věcí úplně postačí.
 * -# ./stm32/f4 Obsahuje třídy a hlavičky pro port na STM32F4 Discovery.
 * 
 * Struktura programu vypadá na první pohled složitě, ale je dost prostá. Základem je třída BaseLayer,
 * pomocí níž jsou propojeny tyto části:
 * @dot
   digraph stack {
     node [style=filled, shape=rectangle, fillcolor=yellow, fontcolor=blue];
     rankdir=LR; rank=same;
     Swdp -> GdbServer -> GdbPacket -> CDClass [dir=both]
   }
 * @enddot
 * -# Swdp zajišťuje fyzický přístup na SWD piny. Je to jeden konec řetězu.
 * -# GdbServer je jádrem celého problému.
 * -# GdbPacket je mezivrstva obsluhující jednotlivé pakety gdb. 
 * -# CDClass je druhý konec řetězce (v PC je to síťový socket),
 * ve vlastním firmware je to opravdu virtuální sériový port.
 * 
 * Jádrem je jak bylo uvedeno GdbServer, bude proto popsán samostatně. Vygeneruj si dokumentaci pomocí
 * <a href="http://www.stack.nl/~dimitri/doxygen/">doxygen</a>. Obsahuje především vlastní
 * Target, který je zde vytvářen dynamicky metodou Scan() - gdb příkaz "monitor scan".
 * 
 * Takže pokud chceme ladit firmware na PC, což je daleko příjemnější, spustíme Makefile v ./dbg a vytvořený
 * firmware.bin nalejeme do LPC11U24 (nebo 34 podle libosti - konečný cíl by měl mít 32kB flash a 8kB ram).
 * V kořenovém Makefile odkomentujeme dáme PLATFORM ?= i386 a spustíme.
 * Výsledný firmware.elf jde pod Linuxem spustit jako gdb server na portu 3333. Tedy pokud máme připojen
 * na USB ten připravený procesor jako /dev/ttyACM0. Takže pokud máme i připojené piny (viz ./lpc11/config.h)
 * na laděný target, funguje to podobně jako OpenOCD. Takto lze ladit vše, co je v ./src (a ./inc).
 * Když máme odladěno, změníme zpět komentáře v Makefile a vytvoříme výsledný firmware.bin, který už
 * má v sobě gdb server. Z postupu je vidět, že C++ umožňuje dost dobře využívat jednou napsaný kód
 * v různých aplikacích prakticky bez zásahu do zdrojáků. Vhodnou strukturou adresářů se lze vyhnout
 * používání těch ifdef, které kód zbytečně znepřehledňují.
 * 
 * Kontrolní sumy vektorů, pro LPC11... dost důležité by měly sedět, takže stačí binárky opravdu jen
 * prostě zkopírovat.
 * 
 * @section sectD Dosažené výsledky.
 * Targety STM32F051 a LPC11U24, 34 jsem mohl otestovat, jiné nemám k dispozizi. Dá se říct, že chodí debug
 * pod gdb dost dobře, lze natáhnout program do ram i flash pomocí "load", gdb si samo určí, kam ho umístí.
 * Krokování delších smyček je problematické, gdb to provádí po jednotlivých instrukcích a po každé instrukci
 * vymění se serverem spoustu dat, takže to trvá dlouho. Protože se to dá pomocí CTRL-C zastavit, zase
 * tak moc to nevadí. Testy by chtěly provést opravdu důkladně, gdb je složité a popravdě všechny příkazy
 * neznám. Běžně vystačím s "run", "continue", "step", "next", "breakpoint", příp. výpisem kusu paměti
 * pomocí "x", "print". Vypisovat registry periférií (na rozdíl od základních registrů procesoru) nejde, xml
 * mapa paměti tuto část neobsahuje. Snad je to tak lépe, ono totiž i pouhé přečtení nějakého registru
 * může změnit chování periferie.
 * 
 * Celé to gdb používám pouze v příkazovém řádku. Těch pár potřebných příkazů se dá snadno naučit a pak
 * je alespoň vidět, co se tam děje, případně kde to zamrzlo. Integrovat to celé do IDE je sice pěkné,
 * ale musíme počítat s tím, že je to rozsáhlý software a mohou v něm být chyby. V IDE pak zjistíme
 * jen to, že to nefunguje. A pak zbývá jen nadávat autorům. Dobré je udělat si v $HOME soubor .gdbinit,
 * ve kterém pak může být např. (psát pořád to "tar ext /home/mrazik/.probe/COM3" je otravné):
 * @code
 * set prompt Kizarm\\>\040
 * # /home/mrazik/.probe/COM3 je link na napr.
 * # /dev/serial/by-id/usb-Mrazik_labs._Kizarm_Probe_1.1_00003-if00
 * tar ext /home/mrazik/.probe/COM3
 * @endcode
 * a pak jednoduše ladíme (STM32F051 v RAM) pomocí
 * @code
    XYZ$ arm-none-eabi-gdb firmware.elf 
    GNU gdb (GNU Tools for ARM Embedded Processors) ... <neni podstatne>
    Reading symbols from /home/mrazik/Public/Arm/Prj/st-cpp/firmware.elf...done.
    Kizarm\> mon scan
    Core Id: 0x0BB11477
    Target: STM32F0xx
    Kizarm\> att 1
    Attaching to program: /home/mrazik/Public/Arm/Prj/st-cpp/firmware.elf, Remote target
    0xfffffffe in ?? ()
    Kizarm\> load
    Loading section .text, size 0xc18 lma 0x20000000
    Loading section .data, size 0x4 lma 0x20000c18
    Loading section ._user_heap_stack, size 0x400 lma 0x20000d10
    Start address 0x20000948, load size 4124
    Transfer rate: 23 KB/sec, 206 bytes/write.
    Kizarm\> r
    The program being debugged has been started already.
    Start it from the beginning? (y or n) y

    Starting program: /home/mrazik/Public/Arm/Prj/st-cpp/firmware.elf 
    ^C
    Program received signal SIGINT, Interrupt.
    0x200002f0 in __WFI () at ../../lib/cmsis/inc/core_cmInstr.h:282
    282       __ASM volatile ("wfi");
    Kizarm\> bt
    #0  0x200002f0 in __WFI () at ../../lib/cmsis/inc/core_cmInstr.h:282
    #1  main () at main.cpp:89
    Kizarm\> det
    Detaching from program: /home/mrazik/Public/Arm/Prj/st-cpp/firmware.elf, Remote target
    Kizarm\> q
 * @endcode
 * 
 * @section sectE Zbývá dodělat.
 * U STM32F051 jsem nezkoušel měnit option byty, snad to funguje. Target STM32F407 funguje také,
 * ale také není úplně otestován (option, flash).
 * Asi by bylo dobře dodělat i target řady LPC8xx, ale zatím to nepotřebuji.
 * 
 * Ten sériový port jsem pokusně přidal jako druhé rozhraní kompozitního zařízení USB. Je to default
 * vypnuto  (1. řádek ./lpc11/makefile.inc), protože ten virtuální sériový port se s ROM drivery
 * chová dost podivně. Nelze přepnout parametry linky (např. baudrate). To sice není moc potřeba,
 * ale chodit by to mělo. Takže tady je slabina. Ostatně původní blackmagic má ten sériový port také
 * poměrně problematický. USB je složité a nevím, jestli jsou vůbec správně deskriptory toho složeného
 * zařízení. Další dost velký problém je, že ROM drivery patrně nepočítají s tím, že by od jedné
 * třídy USB zařízení někdo vytvářel více instancí. Takže zprávy po endpointu 0 chodí zmateně.
 * Zřejmě bude lépe ROM drivery nepoužívat, použít USB stack od Keilu. Virtuální sériový port pokud je
 * potřeba lze udělat jako samostatný firmware - viz. adresář ./com. Stejně bych neuměl přiohnout
 * příslušný inf soubor pro Windows. Takhle lze použít původní NXP.
 * 
 * NXP - Keil drivery jsem konečně vyzkoušel a konstatuji, že to bylo dost práce s diskutabilním výsledkem.
 * Alokace paměti pro endpointy ani zde není průhledná, takže i tady jsou kusy RAM, kde se asi něco
 * děje, ale netušíme co. Alespoň funguje ten endpoint 0. Takže krátce o ladění těchto detailů.
 * Před časem jsem se tím zabýval <a href="http://mujweb.cz/mrazik/usb/index.html">zde</a>.
 * Mezitím se hodně změnilo v Linuxovém jádře, takže celý ten systém funguje asi už úplně jinak.
 * Soubor usbem.c jsem zde ale zachoval, šlo to odladit na starším počítači. Je fakt, že bez toho
 * by to byla práce pro vraha. Výsledek se použít dá (./product/serial.bin), zkoušel jsem to jen na
 * Linuxu (Ubuntu 12.04 LTS), na jiném počítači se starší verzí Ubuntu to občas tuhne, což mohou působit
 * ovladače. Pod Windows jsem nezkoušel ani ten původní firmware s ROM drivery, protoře žádné Windows
 * nemám. Proto ani nepřikládám inf pro instalaci ovladačů. I když loni jsem zkoušel skoro stejný
 * virtuální COM na XP a nebyly s tím žádné problémy. Kdo chce, tak si ten inf na stránkách NXP najde
 * a vyzkouší. Vlastní ovladač je systémový usbser.sys (nebo tak nějak).
 * 
 * Nově doplněna jako platforma na které to může běžet STM32F4 Discovery. Není to sice nic užitečného,
 * protože F4 je dělo na komára, ale ukazuje se, jak snadné je portovat to na jinou architekturu.
 *
 * 
 * @section sectF Zdrojáky.
 * Jsou dostupné na sourceforge jen pomocí subversion :
 * @code
 * svn checkout svn://svn.code.sf.net/p/kizarmprobe/code/ kizarmprobe
 * @endcode
 * 
 * 
 * @section sectG Závěr.
 * Tenhle kus kódu dokumentuje, že lze nacpat do poměrně malého a tedy i levného kontroleru dost
 * složité zařízení. Psát to v C++ se zdá možná zbytečné, ale je to dost užitečné - získá to
 * poněkud na přehlednosti a tedy i rozšířitelnosti. Doufám, že se bude líbit a někomu to pomůže
 * v jeho vlastní činnosti.
 * 
  */