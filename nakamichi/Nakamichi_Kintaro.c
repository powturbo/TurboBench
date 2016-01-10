// Nakamichi is 100% FREE LZSS SUPERFAST decompressor.
// Home of Nakamichi: www.sanmayce.com/Nakamichi/index.html
// Also: http://www.sanmayce.com/Hayabusa/
// Also: http://www.codeproject.com/Articles/878593/Slowest-LZSS-Compressor-in-C

// 'Kintaro' a.k.a. 'Goldenboy' is a 'Shifune' variant with override, 15:1/11:1/7:1 with 24:4/24:3/24:2, also it is purely 64bit decoder - no XMM used.

// How to compile?
/*
D:\_KAZE\Nakamichi_Shifune\Nakamichi_Shifune>type MakeEXEs_GCC_Nakamichi_Shifune.bat
gcc -O3 -fomit-frame-pointer -S Nakamichi_Shifune.c -o Nakamichi_Shifune.S -D_N_XMM -D_N_prefetch_4096 -D_N_Branchfull
gcc -O3 -fomit-frame-pointer Nakamichi_Shifune.c -o Nakamichi_Shifune.exe -D_N_XMM -D_N_prefetch_4096 -D_N_Branchfull

D:\_KAZE\Nakamichi_Shifune\Nakamichi_Shifune>type MakeEXEs_ICC_Nakamichi_Shifune.bat
icl /O3 /QxSSE2 Nakamichi_Shifune.c -D_N_XMM -D_N_prefetch_4096 -D_N_Branchfull /FAcs
if exist Nakamichi_Shifune_branchfull.exe del Nakamichi_Shifune_branchfull.exe
if exist Nakamichi_Shifune_branchfull.cod del Nakamichi_Shifune_branchfull.cod
ren Nakamichi_Shifune.exe Nakamichi_Shifune_branchfull.exe
ren Nakamichi_Shifune.cod Nakamichi_Shifune_branchfull.cod

D:\_KAZE\Nakamichi_Shifune\Nakamichi_Shifune>
*/

// Background:
/*
きんたろう/kintarou/ kintarou/きんたろう/ 【 金太郎 】
noun:
1.Kintaro (hero boy of Japanese folklore, who befriended animals and had supernatural strength)  —Childhood name of late-Heian warrior Sakata no Kintoki. 
2.Kintaro doll (usu. having a plump red face, carrying an axe, and wearing a red apron)
3.diamond-shaped apron
/Source: JMDict/

MASAKARI
鉞
まさかり
broadaxe

The masakari is mainly used to smooth down and finish timber. There are two types of masakari. One has a longer handle and is used mainly for lumbering work, while the other type with a shorter handle, also called the carpenters' broadax (daiku-masakari), is used for rough carpentry work or for making wedges from scraps of wood. 

A carpenter's hewing ax.
This ax was an essential tool for rough hewing work and for smoothing logs to be used as pillars and beams.
It is still used by carpenter's who work on shrines and temples.

Ono and Masakari as religious symbols

The animistic tradition from ancient times state that deities descend to and reside in the mountains. For lumbermen, the mountain was therefore a sacred territory which required strict ritual abstentions to be entered. The ax has been closely related with this religious revering of the mountain and its trees. For example, the first act amongst the myriad of Shinto rituals carried out before the lumbering for the rebuilding of the Ise Shrine every 20 years, is the cutting into a tree with a ritually purified ax (imi-ono). Moreover in the festival of the pillar (Onbashira-matsuri) at the Suwa shrine, a vermillion-lacquered ax is used to cut down a tree which is to become the sacred pillar.

In Buddhist symbolism the ax also acquires the power of cutting off evil, and there are numerous existing statues of bodhisattva holding axes. Shugen-do, a traditional Japanese religion born out of an amalgam of different religions including Shintoism and Buddhism which has a particular connection with mountains, regards the ax as one of the symbolic objects to be carried by practitioners when going into mountains for ascetic training.

Ax is also an important (heavenly/carpentry) instrument in Laoism, a few excerpts from pseudo-chapter 74 of 'Dao De Jing':

Translation: Lin Yutang
And to take the place of the executioner
Is like handling the hatchet for the master carpenter.
He who handles the hatchet for the master carpenter
Seldom escapes injury to his hands.

Translation: Gu Zhengkun
If one wants to kill on behalf of the executioner,
It is like chopping wood on behalf of the master carpenter.
There are few who can escape cutting their own hands
When they chop wood on behalf of the master carpenter.

Translation: Ch'u Ta-Kao
Only the Supreme Executioner kills.
To kill in place of the Supreme Executioner is to hack instead of a greater carpenter.
Now if one hacks in place of a great carpenter one can scarcely avoid cutting one's own hand.

Translation: Hua-Ching Ni
To become the executioner of artificial righteousness is like the inexperienced lad who would brandish a sharp axe of a master carpenter.
He can seldom escape cutting himself.

Translation: Witter Bynner
Nature is executioner.
When man usurps the place,
A carpenter's apprentice takes the place of the master:
And 'an apprentice hacking with the master's axe
May slice his own hand.'

Translation: Hu Xuezhi
Therefore, there always exists the executioner in charge of inflicting death.
To inflict death in place of the executioner,
Is equal to finishing an excellent woodwork in place of a master carpenter.
Of those who finish an excellent woodwork in place of a master carpenter,
Few will not hurt their hands.

Translation: J.J.L. Duyvendak
There is always a Chief Executioner who kills.
To kill in place of the Chief Executioner may be called: to chop in place of the Master Carpenter.
Now in chopping in place of the Master Carpenter, few will escape maiming their hands!

Translation: Spurgeon Medhurst
There is one who inflicts sentence of death.
To usurp his functions and to kill would be to assume the role of the Master-Carpenter.
There are few who can act as Master-Carpenter without cutting their hands.

Translation: Richard Wilhelm
There is always a power of death that kills.
To kill instead of leaving killing to this power of death
is as if one wanted to use the axe oneself
instead of leaving it to the carpenter.
Whosoever would use the axe
instead of leaving it to the carpenter
shall rarely get away
without injuring his hand.

KINTARO - From the folklore of Japan - Legendary symbol of virtue and strength

Kintaro is a beloved legendary and symbolic figure from Japan. Like many legendary figures he appears in both history and mythology.
According to classic Japanese literature he was fathered by a great Red Dragon ( the thunder god - see below ) who visited his mountain sorceress mother in a dream.
She awoke amidst powerful claps of thunder and knew at once she was with child.
Kintaro means "Golden Boy" and his jealous uncle sought to kill him.
His mother took him and fled into the Hakone mountains to the deepest forests of Mount Kintoki.
Growing up deep in the forest his beautiful spirit caused him to become a special friend to all the wild animals, most especially the rabbits and the bears.
He loved to play with his animal friends about the rocks of the Yuhi no Taki Falls.
So strong was he as a boy and so gifted at Sumo wrestling that he could throw down a bear.
He was a very good boy, rosy-cheeked and chubby and always carried a hatchet, the Japanese symbol of the thunder god and is usually depicted riding his beloved bear.

This documentlet is available thanks to sources:
1]
TAKENAKA CARPENTRY TOOLS MUSEUM
4-18-25, Nakayamate-dori, Chuo-ku,
Kobe 650-0004, Japan
2]
The Asian Myths & Legends Art page of Howard David Johnson
3]
Japanese Architecture and Art Net Users System
4]
My web-page: www.sanmayce.com

Kaze,
2013-Jan-07
*/

// 'Kintaro' at a glance:
/*

*/

// 'Shifune/Deathship' at a glance:
/*
Intel(R) Parallel Studio XE 2015
Copyright (C) 1985-2014 Intel Corporation. All rights reserved.

Intel(R) MPI Library 5.0 Update 1 Build Environment for Intel(R) 64 applications
Copyright (C) 2007-2014 Intel Corporation. All rights reserved.

Intel(R) Trace Analyzer and Collector 9.0 Update 1 for Windows* OS for Intel(R) 64 applications
Copyright (C) 1996-2014 Intel Corporation. All rights reserved.

Setting environment for using Microsoft Visual Studio 2010 x64 cross tools.

C:\Program Files (x86)\Intel>cd "Composer XE 2015"

C:\Program Files (x86)\Intel\Composer XE 2015>cd bin

C:\Program Files (x86)\Intel\Composer XE 2015\bin>iclvars.bat intel64
Intel(R) Parallel Studio XE 2015
Copyright (C) 1985-2014 Intel Corporation. All rights reserved.
Intel(R) Parallel Studio XE 2015 Composer Edition (package 108)
Setting environment for using Microsoft Visual Studio 2010 x64 tools.

C:\Program Files (x86)\Intel\Composer XE 2015\bin>d:

D:\>cd D:\_KAZE\Nakamichi_Shifune\Initial

D:\_KAZE\Nakamichi_Shifune\Initial>dir

08/17/2015  11:04 PM        33,258,496 Agatha_Christie_89-ebooks_TXT.tar
08/17/2015  09:12 PM        10,623,335 Agatha_Christie_89-ebooks_TXT.tar.Nakamichi
08/17/2015  11:04 PM        10,192,446 dickens
08/17/2015  02:55 AM         3,740,418 dickens.Nakamichi
08/17/2015  11:04 PM         1,820,160 Fleurs_du_mal.tar
08/16/2015  11:59 PM           540,320 Fleurs_du_mal.tar.Nakamichi
08/17/2015  11:04 PM         3,265,536 University_of_Canterbury_The_Calgary_Corpus.tar
08/16/2015  11:10 PM         1,319,701 University_of_Canterbury_The_Calgary_Corpus.tar.Nakamichi
...
08/16/2015  10:14 PM               367 MakeEXEs_Nakamichi_Shifune.bat
08/19/2015  10:20 PM           176,084 Nakamichi_Shifune.c
08/19/2015  10:21 PM           715,290 Nakamichi_Shifune_branchfull.cod
08/19/2015  10:21 PM           120,832 Nakamichi_Shifune_branchfull.exe

D:\_KAZE\Nakamichi_Shifune\Initial>Nakamichi_Shifune_branchfull.exe Agatha_Christie_89-ebooks_TXT.tar.Nakamichi /report
Nakamichi 'Shifune-Totenschiff', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 10623335 bytes ...
RAM-to-RAM performance: 320 MB/s.
Memory pool starting address: 0000000001080080 ... 64 byte aligned, OK
Copying a 512MB block 1024 times i.e. 512GB READ + 512GB WRITTEN ...
^C
D:\_KAZE\Nakamichi_Shifune\Initial>Nakamichi_Shifune_branchfull.exe dickens.Nakamichi /report
Nakamichi 'Shifune-Totenschiff', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 3740418 bytes ...
RAM-to-RAM performance: 512 MB/s.
Memory pool starting address: 0000000000830080 ... 64 byte aligned, OK
Copying a 512MB block 1024 times i.e. 512GB READ + 512GB WRITTEN ...
^C
D:\_KAZE\Nakamichi_Shifune\Initial>Nakamichi_Shifune_branchfull.exe Fleurs_du_mal.tar.Nakamichi /report
Nakamichi 'Shifune-Totenschiff', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 540320 bytes ...
RAM-to-RAM performance: 768 MB/s.
Memory pool starting address: 0000000000560080 ... 64 byte aligned, OK
Copying a 512MB block 1024 times i.e. 512GB READ + 512GB WRITTEN ...
^C
D:\_KAZE\Nakamichi_Shifune\Initial>Nakamichi_Shifune_branchfull.exe University_of_Canterbury_The_Calgary_Corpus.tar.Nakamichi /report
Nakamichi 'Shifune-Totenschiff', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 1319701 bytes ...
RAM-to-RAM performance: 576 MB/s.
Memory pool starting address: 00000000006F0080 ... 64 byte aligned, OK
Copying a 512MB block 1024 times i.e. 512GB READ + 512GB WRITTEN ...
^C
*/

/*
Quick  English texts  decompression  showdown

- 1st column: Intel Core 2 Q9550s 'Yorkfield' @2833MHz 45nm, Cache 12MB, RAM @666MHz DDR2 CL5 CR2, laptop Asus;
- 2nd column: Intel i7 4960x 'Ivy Bridge E' 22nm, Cache 15MB, core @4600MHz RAM @2133MHz 8-10-10-21 CR1; MOBO Asus Rampage IV Black Edition;
  http://cdn.overclock.net/8/8b/8b9d96b9_decompshowdwn.jpeg
- 3rd column: Intel i7 5960x 'Haswell E/EP' 22nm, Cache 20MB, core/cache @4700MHz/@4200MHz RAM @2666MHz; MOBO Asus Rampage V Extreme;
  http://cdn.overclock.net/8/8b/8b2c8f32_decompshowdown.jpeg
- 4th column: Intel Core i5 6600K 'Skylake' 14nm, Cache 6MB, core/cache @4700MHz/4500MHz RAM @3200MHz CL15; MOBO Asus Maximus VIII Extreme;
  http://cdn.overclock.net/6/6b/6b1770b9_decompshowdown.jpeg

-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|  Ratio |                                                                                     testdatafile |    Yorkfield | Ivy Bridge E | Haswell E/EP |      Skylake |
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|        |  13,713,275 Complete_Works_of_Fyodor_Dostoyevsky.txt                                             |              |              |              |              |
|        |   3,504,471 Complete_Works_of_Fyodor_Dostoyevsky.txt.256MB.lzturbo12-39.lzt                      |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   3,717,191 Complete_Works_of_Fyodor_Dostoyevsky.txt.4MB.lzturbo12-39.lzt                        |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   3,717,583 Complete_Works_of_Fyodor_Dostoyevsky.txt.brotli                                      | 153.228 MB/s | 346.897 MB/s |  364.29 MB/s | 397.508 MB/s |
| 2.99:1 |   4,582,363 Complete_Works_of_Fyodor_Dostoyevsky.txt.Nakamichi                                   |     448 MB/s |    1408 MB/s |    2112 MB/s |    1728 MB/s |
|        |   4,617,360 Complete_Works_of_Fyodor_Dostoyevsky.txt.zip                                         |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   5,209,670 Complete_Works_of_Fyodor_Dostoyevsky.txt.zst                                         |   302.5 MB/s |   588.9 MB/s |   619.8 MB/s |   628.3 MB/s |
|        |   4,218,337 Complete_Works_of_Fyodor_Dostoyevsky.txt.method28.zpaq                               |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   2,876,526 Complete_Works_of_Fyodor_Dostoyevsky.txt.ST6Block256.bsc                             |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   3,474,224 Complete_Works_of_Fyodor_Dostoyevsky.txt.xz                                          |         N.A. |         N.A. |         N.A. |         N.A. |
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|        |  10,192,446 dickens                                                                              |              |              |              |              |
|        |   2,861,165 dickens.256MB.lzturbo12-39.lzt                                                       |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   2,976,910 dickens.4MB.lzturbo12-39.lzt                                                         |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   2,962,118 dickens.brotli                                                                       |  146.61 MB/s | 313.557 MB/s | 336.342 MB/s | 374.577 MB/s |
| 2.72:1 |   3,740,418 dickens.Nakamichi                                                                    |     448 MB/s |    1344 MB/s |    1984 MB/s |    1664 MB/s |
|        |   3,681,828 dickens.zip                                                                          |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   4,134,924 dickens.zst                                                                          |   298.9 MB/s |   587.1 MB/s |   619.7 MB/s |   623.8 MB/s |
|        |   3,407,946 dickens.method28.zpaq                                                                |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   2,319,874 dickens.ST6Block256.bsc                                                              |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   2,831,212 dickens.xz                                                                           |         N.A. |         N.A. |         N.A. |         N.A. |
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|        | 100,000,000 enwik8                                                                               |              |              |              |              |
|        |  25,330,833 enwik8.256MB.lzturbo12-39.lzt                                                        |         N.A. |         N.A. |         N.A. |         N.A. |
|        |  29,148,393 enwik8.4MB.lzturbo12-39.lzt                                                          |         N.A. |         N.A. |         N.A. |         N.A. |
|        |  27,722,164 enwik8.brotli                                                                        | 134.094 MB/s | 266.911 MB/s |  285.19 MB/s | 305.225 MB/s |
| 2.92:1 |  34,218,460 enwik8.Nakamichi                                                                     |     256 MB/s |     960 MB/s |    1024 MB/s |    1152 MB/s |
|        |  35,102,891 enwik8.zip                                                                           |         N.A. |         N.A. |         N.A. |         N.A. |
|        |  40,024,854 enwik8.zst                                                                           |   325.0 MB/s |   620.6 MB/s |   651.6 MB/s |   653.8 MB/s |
|        |  30,088,258 enwik8.method28.zpaq                                                                 |         N.A. |         N.A. |         N.A. |         N.A. |
|        |  22,112,692 enwik8.ST6Block256.bsc                                                               |         N.A. |         N.A. |         N.A. |         N.A. |
|        |  24,831,648 enwik8.xz                                                                            |         N.A. |         N.A. |         N.A. |         N.A. |
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|        |  14,613,183 The_Book_of_The_Thousand_Nights_and_a_Night.txt                                      |              |              |              |              |
|        |   3,982,547 The_Book_of_The_Thousand_Nights_and_a_Night.txt.256MB.lzturbo12-39.lzt               |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   4,241,855 The_Book_of_The_Thousand_Nights_and_a_Night.txt.4MB.lzturbo12-39.lzt                 |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   4,163,630 The_Book_of_The_Thousand_Nights_and_a_Night.txt.brotli                               | 144.867 MB/s | 321.111 MB/s | 336.624 MB/s | 372.129 MB/s |
| 2.76:1 |   5,293,102 The_Book_of_The_Thousand_Nights_and_a_Night.txt.Nakamichi                            |     384 MB/s |    1728 MB/s |    1984 MB/s |    1600 MB/s |
|        |   5,198,949 The_Book_of_The_Thousand_Nights_and_a_Night.txt.zip                                  |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   5,932,453 The_Book_of_The_Thousand_Nights_and_a_Night.txt.zst                                  |   305.8 MB/s |   595.3 MB/s |   625.4 MB/s |   631.3 MB/s |
|        |   4,717,755 The_Book_of_The_Thousand_Nights_and_a_Night.txt.method28.zpaq                        |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   3,273,062 The_Book_of_The_Thousand_Nights_and_a_Night.txt.ST6Block256.bsc                      |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   3,913,236 The_Book_of_The_Thousand_Nights_and_a_Night.txt.xz                                   |         N.A. |         N.A. |         N.A. |         N.A. |
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|        |   4,445,260 The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt                        |              |              |              |              |
|        |   1,076,637 The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt.256MB.lzturbo12-39.lzt |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   1,089,279 The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt.4MB.lzturbo12-39.lzt   |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   1,087,439 The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt.brotli                 | 160.277 MB/s | 304.988 MB/s | 339.146 MB/s | 370.247 MB/s |
| 3.08:1 |   1,441,679 The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt.Nakamichi              |     704 MB/s |    2048 MB/s |    2432 MB/s |    2368 MB/s |
|        |   1,320,100 The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt.zip                    |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   1,537,047 The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt.zst                    |   320.7 MB/s |   622.4 MB/s |   656.1 MB/s |   668.6 MB/s |
|        |   1,286,796 The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt.method28.zpaq          |         N.A. |         N.A. |         N.A. |         N.A. |
|        |     855,786 The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt.ST6Block256.bsc        |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   1,056,804 The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt.xz                     |         N.A. |         N.A. |         N.A. |         N.A. |
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|        |   3,265,536 University_of_Canterbury_The_Calgary_Corpus.tar                                      |              |              |              |              |
|        |     921,958 University_of_Canterbury_The_Calgary_Corpus.tar.256MB.lzturbo12-39.lzt               |         N.A. |         N.A. |         N.A. |         N.A. |
|        |     921,958 University_of_Canterbury_The_Calgary_Corpus.tar.4MB.lzturbo12-39.lzt                 |         N.A. |         N.A. |         N.A. |         N.A. |
|        |     867,503 University_of_Canterbury_The_Calgary_Corpus.tar.brotli                               | 144.849 MB/s | 245.217 MB/s | 266.176 MB/s | 283.114 MB/s |
| 2.47:1 |   1,319,701 University_of_Canterbury_The_Calgary_Corpus.tar.Nakamichi                            |     576 MB/s |    1600 MB/s |    1792 MB/s |    1792 MB/s |
|        |   1,017,658 University_of_Canterbury_The_Calgary_Corpus.tar.zip                                  |         N.A. |         N.A. |         N.A. |         N.A. |
|        |   1,174,349 University_of_Canterbury_The_Calgary_Corpus.tar.zst                                  |   367.3 MB/s |   704.2 MB/s |   740.6 MB/s |   747.2 MB/s |
|        |   1,079,861 University_of_Canterbury_The_Calgary_Corpus.tar.method28.zpaq                        |         N.A. |         N.A. |         N.A. |         N.A. |
|        |     825,124 University_of_Canterbury_The_Calgary_Corpus.tar.ST6Block256.bsc                      |         N.A. |         N.A. |         N.A. |         N.A. |
|        |     851,328 University_of_Canterbury_The_Calgary_Corpus.tar.xz                                   |         N.A. |         N.A. |         N.A. |         N.A. |
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------

[Performers:]

- lzturbo 1.2 Copyright (c) 2007-2014 Hamid Buzidi, Aug 11 2014.
- brotli, 25-Sep-2015 commit, compiled (with several syntactic changes) with Intel v15.0 optimizer (/O3 used).
- Nakamichi 'Shifune-Totenschiff', written by Kaze, based on Nobuo Ito's LZSS source, 2015-Sep-02.
- 7-Zip (A) 9.20, Copyright (c) 1999-2010 Igor Pavlov, 2010-11-18.
- zstd command line interface 64-bits v0.0.1, by Yann Collet (Jan 25 2015).
- zpaq v7.05 journaling archiver, compiled Apr 17 2015.
- bsc, Block Sorting Compressor, Version 3.1.0. Copyright (c) 2009-2012 Ilya Grebnov, 8 July 2012.
- xz (XZ Utils) 5.2.1, Lasse Collin and Igor Pavlov, 2015-02-26.

[Options used:]

FOR /F %%G IN (DIRLIST) DO lzturbo.exe -39 -b256 -p0 %%G %%G.256MB.lzturbo12-39.lzt
FOR /F %%G IN (DIRLIST) DO lzturbo.exe -39 -b4 -p0 %%G %%G.4MB.lzturbo12-39.lzt
FOR /F %%G IN (DIRLIST) DO bro_Intel15 -i %%G -o %%G.brotli -v -q 11
FOR /F %%G IN (DIRLIST) DO Nakamichi_Shifune_branchfull %%G
FOR /F %%G IN (DIRLIST) DO 7za a -tgzip -mx9 %%G.zip %%G
FOR /F %%G IN (DIRLIST) DO zstd.exe %%G
FOR /F %%G IN (DIRLIST) DO zpaq64 add %%G.method28.zpaq %%G -method 28 -threads 1
FOR /F %%G IN (DIRLIST) DO bsc e %%G %%G.ST6Block256.bsc -b256 -m6 -cp -Tt
FOR /F %%G IN (DIRLIST) DO xz -z -k -f -9 -e -v -v --threads=1 %%G

Note1: On Linux using 'dickens', Brotli vs Shifune - 195.78 MB/s vs 960 MB/s, using AMD FX8350 @4.0GHz.
       http://www.cnx-software.com/2015/09/24/brotli-compression-algorithm-combines-high-compression-ratio-and-fast-decompression/
Note2: Zstd is yet to kick asses - especially when multi-threaded.
Note3: Special thanks go to the Philadelphian overclocker Jpmboy.
*/

/*
10/06/2014 02:06 AM  48,194,287 Autobiography_411-ebooks_Collection.tar.paq8hp12 ! -8 !
10/06/2014 07:16 PM  52,631,060 Autobiography_411-ebooks_Collection.tar.cm.nz ! -cc !
06/14/2015 03:52 AM  53,597,471 Autobiography_411-ebooks_Collection.tar.method58.zpaq
10/05/2014 07:15 AM  54,635,921 Autobiography_411-ebooks_Collection.tar.order16.PPMonstr ! -m1024 -o16 !
10/05/2014 07:26 AM  55,011,526 Autobiography_411-ebooks_Collection.tar.order32.PPMonstr ! -m1024 -o32 !
10/06/2014 03:02 AM  55,117,057 Autobiography_411-ebooks_Collection.tar.method5.zpaq / zpaq 6.50 /
10/06/2014 03:33 AM  55,464,039 Autobiography_411-ebooks_Collection.tar.BWT_block256.bsc ! -m3 -Tt -b256 !
10/05/2014 07:05 AM  55,962,342 Autobiography_411-ebooks_Collection.tar.order08.PPMonstr ! -m1024 -o8 !
10/05/2014 07:41 AM  56,862,830 Autobiography_411-ebooks_Collection.tar.bbb ! cfm128 !
10/06/2014 02:32 AM  57,962,141 Autobiography_411-ebooks_Collection.tar.method4.zpaq / zpaq 6.50 /
10/05/2014 02:55 AM  58,266,061 Autobiography_411-ebooks_Collection.tar.tangelo / Version 2.3 /
06/14/2015 06:07 AM  58,631,951 Autobiography_411-ebooks_Collection.tar.method53.zpaq
10/05/2014 07:00 AM  58,736,456 Autobiography_411-ebooks_Collection.tar.order06.PPMonstr ! -m1024 -o6 !
06/14/2015 03:52 AM  59,360,768 Autobiography_411-ebooks_Collection.tar.ST6Block256.bsc
10/06/2014 02:27 AM  59,401,801 Autobiography_411-ebooks_Collection.tar.method3.zpaq / zpaq 6.50 /
10/06/2014 03:36 AM  61,018,244 Autobiography_411-ebooks_Collection.tar.ST5_block256.bsc ! -m2 -Tt -b256 !
06/14/2015 05:53 AM  64,210,456 Autobiography_411-ebooks_Collection.tar.ST6Block008.bsc
06/16/2015 02:29 AM  64,501,642 Autobiography_411-ebooks_Collection.tar.WinZip_19.5.ep.maximum(PPMd).zipx
10/06/2014 03:35 AM  65,416,196 Autobiography_411-ebooks_Collection.tar.ST4_block256.bsc ! -m1 -Tt -b256 !
10/05/2014 06:56 AM  66,517,316 Autobiography_411-ebooks_Collection.tar.order04.PPMonstr ! -m1024 -o4 !
10/06/2014 08:36 PM  67,946,358 Autobiography_411-ebooks_Collection.tar.v1.2_49_block256.lzt ! -49 -b256 -p1 !
10/06/2014 07:57 PM  69,398,819 Autobiography_411-ebooks_Collection.tar.v1.2_39_block256.lzt ! -39 -b256 -p1 !
10/05/2014 06:53 AM  69,832,119 Autobiography_411-ebooks_Collection.tar.7z ! -t7z -mx9 !
06/16/2015 02:27 AM  72,167,788 Autobiography_411-ebooks_Collection.tar.WinZip_19.5.el.maximum(LZMA).zipx
10/06/2014 03:34 AM  75,388,190 Autobiography_411-ebooks_Collection.tar.ST3_block256.bsc ! -m0 -Tt -b256 !
06/16/2015 02:23 AM  75,506,105 Autobiography_411-ebooks_Collection.tar.WinZip_19.5.eb.maximum(bzip2).zipx
10/06/2014 07:18 PM  77,194,175 Autobiography_411-ebooks_Collection.tar.lzhds.nz ! -cD !
06/14/2015 06:11 AM  77,248,102 Autobiography_411-ebooks_Collection.tar.008MB.lzturbo12-39.lzt
10/06/2014 02:21 AM  82,724,804 Autobiography_411-ebooks_Collection.tar.sr2
10/09/2015 01:32 PM  86,758,638 Autobiography_411-ebooks_Collection.tar.Shifune.Nakamichi
10/06/2014 02:25 AM  87,439,698 Autobiography_411-ebooks_Collection.tar.method2.zpaq / zpaq 6.50 /
06/14/2015 03:52 AM  88,867,460 Autobiography_411-ebooks_Collection.tar.Hoshimi.Nakamichi
06/14/2015 03:52 AM  91,964,279 Autobiography_411-ebooks_Collection.tar.Lexx.Nakamichi / If one-char-look-ahead is applied as in ‘Tengu-Tsuyo’ then better compression /
10/05/2014 06:47 AM  97,569,200 Autobiography_411-ebooks_Collection.tar.zip ! -tzip -mx9 !
06/16/2015 02:29 AM  97,791,851 Autobiography_411-ebooks_Collection.tar.WinZip_19.5.ee.maximum(enhanced_deflate).zipx
10/06/2014 02:23 AM 101,966,054 Autobiography_411-ebooks_Collection.tar.method1.zpaq / zpaq 6.50 /
06/16/2015 02:29 AM 102,146,322 Autobiography_411-ebooks_Collection.tar.WinZip_19.5.ex.maximum(portable).zip
10/06/2014 04:06 AM 102,514,628 Autobiography_411-ebooks_Collection.tar.lzh / LHA32 version 2.67 /
07/17/2015 07:18 PM 106,365,732 Autobiography_411-ebooks_Collection.tar.Tengu-Tsuyo.Nakamichi
10/05/2014 09:12 AM 107,237,997 Autobiography_411-ebooks_Collection.tar.Tengu.Nakamichi
06/14/2015 03:52 AM 109,757,342 Autobiography_411-ebooks_Collection.tar.zst
10/05/2014 03:23 AM 113,511,873 Autobiography_411-ebooks_Collection.tar.Z
10/06/2014 07:43 PM 115,558,538 Autobiography_411-ebooks_Collection.tar.v1.2_19.lzt ! -19 !
10/05/2014 06:54 AM 117,126,206 Autobiography_411-ebooks_Collection.tar.lz4 ! -9 !
06/13/2015 02:21 PM 273,401,856 Autobiography_411-ebooks_Collection.tar

D:\_Z\NL>Nakamichi_Shifune_branchfull.exe Autobiography_411-ebooks_Collection.tar
Nakamichi 'Shifune-Totenschiff', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Compressing 273401856 bytes ...
\; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 787
NumberOfFlushLiteralsHeuristic (bigger-the-better): 1272005
Legend: WindowSizes: 1/2/3/4=Tiny/Short/Medium/Long
NumberOf(Short)Matches[Tiny]Window (7)[1]: 20538
NumberOf(Medium)Matches[Tiny]Window (11)[1]: 3483
NumberOf(Long)Matches[Tiny]Window (15)[1]: 16107
NumberOf(Tiny)Matches[Short]Window (4)[2]: 1740764
NumberOf(Short)Matches[Short]Window (8)[2]: 1299820
NumberOf(Medium)Matches[Short]Window (12)[2]: 566491
NumberOf(Long)Matches[Short]Window (16)[2]: 343448
NumberOf(Tiny)Matches[Medium]Window (5)[3]: 2270284
NumberOf(Short)Matches[Medium]Window (9)[3]: 3685139
NumberOf(Medium)Matches[Medium]Window (13)[3]: 1673007
NumberOf(Long)Matches[Medium]Window (17)[3]: 1012877
NumberOf(Tiny)Matches[Long]Window (6)[4]: 968932
NumberOf(Short)Matches[Long]Window (10)[4]: 4167887
NumberOf(Medium)Matches[Long]Window (14)[4]: 4010356
NumberOf(Long)Matches[Long]Window (18)[4]: 2980637
RAM-to-RAM performance: 4399 bytes/s.
Compressed to 86758638 bytes.

1-threaded equals mono-threaded
2-threaded equals di-threaded
4-threaded equals tetra-threaded
8-threaded equals octa-threaded
12-threaded equals dodeca-threaded
16-threaded equals hexadeca-threaded
18-threaded equals octadeca-threaded
32-threaded equals dotriaconta-threaded
64-threaded equals tetrahexaconta-threaded
128-threaded equals octacosahecta-threaded
256-threaded equals hexapentacontadicta-threaded
*/

/*
D:\_KAZE\Nakamichi_Shifune\Initial>Nakamichi_Shifune_branchfull.exe enwik8
Nakamichi 'Shifune-Totenschiff', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Compressing 100000000 bytes ...
/; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 3798
NumberOfFlushLiteralsHeuristic (bigger-the-better): 630829
Legend: WindowSizes: 1/2/3/4=Tiny/Short/Medium/Long
NumberOf(Short)Matches[Tiny]Window (7)[1]: 29945
NumberOf(Medium)Matches[Tiny]Window (11)[1]: 7876
NumberOf(Long)Matches[Tiny]Window (15)[1]: 472
NumberOf(Tiny)Matches[Short]Window (4)[2]: 1108606
NumberOf(Short)Matches[Short]Window (8)[2]: 764876
NumberOf(Medium)Matches[Short]Window (12)[2]: 409264
NumberOf(Long)Matches[Short]Window (16)[2]: 545020
NumberOf(Tiny)Matches[Medium]Window (5)[3]: 1280412
NumberOf(Short)Matches[Medium]Window (9)[3]: 1138105
NumberOf(Medium)Matches[Medium]Window (13)[3]: 426568
NumberOf(Long)Matches[Medium]Window (17)[3]: 386077
NumberOf(Tiny)Matches[Long]Window (6)[4]: 688726
NumberOf(Short)Matches[Long]Window (10)[4]: 1222970
NumberOf(Medium)Matches[Long]Window (14)[4]: 900059
NumberOf(Long)Matches[Long]Window (18)[4]: 828912
RAM-to-RAM performance: 147 bytes/s.
Compressed to 34218460 bytes.

D:\_KAZE\Nakamichi_Shifune\Nakamichi_Shifune>Nakamichi_Shifune_MinGW_510.exe enwik8.Nakamichi /report
Nakamichi 'Shifune-Totenschiff', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 34218460 bytes ...
RAM-to-RAM performance: 256 MB/s.
Memory pool starting address: 0000000002700080 ... 64 byte aligned, OK
Copying a 512MB block 1024 times i.e. 512GB READ + 512GB WRITTEN ...
^C
D:\_KAZE\Nakamichi_Shifune\Nakamichi_Shifune>Nakamichi_Shifune_branchfull.exe enwik8.Nakamichi /report
Nakamichi 'Shifune-Totenschiff', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 34218460 bytes ...
RAM-to-RAM performance: 256 MB/s.
Memory pool starting address: 0000000002500080 ... 64 byte aligned, OK
Copying a 512MB block 1024 times i.e. 512GB READ + 512GB WRITTEN ...
^C
D:\_KAZE\Nakamichi_Shifune\Nakamichi_Shifune>
*/

/*
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| testdatafile \ decompressor                                           | Uncompressed |            ZSTD v0.0.1 | LZ4 v1.4 (-9 -b -Sx -T1) |   Nakamichi 'Shifune' | 7za a -tgzip -mx9 |
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| The_Project_Gutenberg_EBook_of_Gulliver's_Travels_gltrv10.txt         |      604,629 |    239,149; 300.5 MB/s |     261,041;  927.1 MB/s |     xxx,xxx; zzz MB/s |     214,817; N.A. |
| The_Project_Gutenberg_EBook_of_Notre-Dame_de_Paris_2610-8.txt         |    1,101,276 |    450,066; 301.9 MB/s |     489,223;  901.5 MB/s |     xxx,xxx; zzz MB/s |     407,215; N.A. |
| The_Project_Gutenberg_EBook_of_Moby_Dick_moby11.txt                   |    1,255,801 |    540,039; 299.3 MB/s |     588,381;  895.7 MB/s |     xxx,xxx; zzz MB/s |     483,065; N.A. |
| Fleurs_du_mal.tar                                                     |    1,820,160 |    372,893; 563.3 MB/s |     607,482; 1139.3 MB/s |     540,320; 768 MB/s |     496,964; N.A. |
| pg46853_Le_Morte_Darthur_by_Sir_Thomas_Malory.txt                     |    2,136,831 |    733,590; 319.5 MB/s |     783,810;  943.7 MB/s |     xxx,xxx; zzz MB/s |     647,186; N.A. |
| University_of_Canterbury_The_Calgary_Corpus.tar                       |    3,265,536 |  1,164,397; 368.2 MB/s |   1,241,281; 1055.0 MB/s |   1,319,701; 576 MB/s |   1,017,658; N.A. |
| The_Complete_Sherlock_Holmes_-_Doyle_Arthur_Conan.txt                 |    3,714,387 |  1,422,283; 302.6 MB/s |   1,539,507;  915.7 MB/s |   x,xxx,xxx; zzz MB/s |   1,285,462; N.A. |
| The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt         |    4,445,260 |  1,512,215; 322.2 MB/s |   1,605,032;  952.2 MB/s |   1,441,679; 704 MB/s |   1,320,100; N.A. |
| Ian_Fleming_-_The_James_Bond_Anthology_(complete_collection).epub.txt |    5,245,293 |  2,079,270; 298.7 MB/s |   2,256,844;  899.4 MB/s |   1,938,723; 640 MB/s |   1,869,849; N.A. |
| Complete_Works_of_William_Shakespeare.txt                             |   10,455,117 |  3,851,884; 313.7 MB/s |   4,200,287;  908.9 MB/s |   x,xxx,xxx; zzz MB/s |   3,378,656; N.A. |
| Complete_Works_of_Fyodor_Dostoyevsky.txt                              |   13,713,275 |  5,127,549; 303.2 MB/s |   5,569,991;  906.5 MB/s |   4,582,363; 448 MB/s |   4,617,360; N.A. |
| The_Book_of_The_Thousand_Nights_and_a_Night.txt                       |   14,613,183 |  5,855,516; 306.2 MB/s |   6,223,909;  914.7 MB/s |   5,293,102; 384 MB/s |   5,198,949; N.A. |
| Dune_Complete_17_Ebooks.tar                                           |   16,973,312 |  6,660,569; 299.5 MB/s |   7,290,214;  894.2 MB/s |   5,893,697; 384 MB/s |   6,086,933; N.A. |
| Agatha_Christie_89-ebooks_TXT.tar                                     |   33,258,496 | 12,404,504; 305.5 MB/s |  13,365,090;  909.2 MB/s |  10,623,335; 320 MB/s |  11,173,195; N.A. |
| Encyclopedia_of_Language_and_Linguistics.txt                          |   59,416,161 | 19,435,030; 358.7 MB/s |  21,118,907; 1038.8 MB/s |  18,502,271; 320 MB/s |  17,546,530; N.A. |
| Stephen_King_67-books.tar                                             |   61,382,144 | 24,402,931; 299.2 MB/s |  26,310,486;  894.3 MB/s |  20,350,142; 256 MB/s |  21,854,632; N.A. |
| enwik8                                                                |  100,000,000 | 39,573,323; 325.6 MB/s |  42,283,793;  936.9 MB/s |  34,218,460; 256 MB/s |  35,102,891; N.A. |
| New_Shorter_Oxford_English_Dictionary_fifth_edition.tar               |  132,728,832 | 28,968,421; 460.7 MB/s |  30,133,137; 1169.4 MB/s |  29,059,023; 448 MB/s |  25,418,601; N.A. |
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/

/*
Long after the 'White Fox' benchmark here comes the latest most optimized (for English texts) decompression benchmark called 'Crazy Horse', downloadable here. https://mega.nz/#!J15U0JTQ!FC7zC-CRdRiHRypkVuaveosIHeSHIvxbNYpJj5mAl8U

On Core 2 Q9550s @2.83GHz:

---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| testdatafile \ decompressor                                           | Uncompressed |            ZSTD v0.0.1 | LZ4 v1.4 (-9 -b -Sx -T1) |   Nakamichi 'Akuuma' | 7za a -tgzip -mx9 |
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| The_Project_Gutenberg_EBook_of_Gulliver's_Travels_gltrv10.txt         |      604,629 |    239,149; 300.5 MB/s |     261,041;  927.1 MB/s |    263,629; 576 MB/s |     214,817; N.A. |
| The_Project_Gutenberg_EBook_of_Notre-Dame_de_Paris_2610-8.txt         |    1,101,276 |    450,066; 301.9 MB/s |     489,223;  901.5 MB/s |    472,872; 576 MB/s |     407,215; N.A. |
| The_Project_Gutenberg_EBook_of_Moby_Dick_moby11.txt                   |    1,255,801 |    540,039; 299.3 MB/s |     588,381;  895.7 MB/s |    568,117; 512 MB/s |     483,065; N.A. |
| Fleurs_du_mal.tar                                                     |    1,820,160 |    372,893; 563.3 MB/s |     607,482; 1139.3 MB/s |    551,658; 832 MB/s |     496,964; N.A. |
| pg46853_Le_Morte_Darthur_by_Sir_Thomas_Malory.txt                     |    2,136,831 |    733,590; 319.5 MB/s |     783,810;  943.7 MB/s |    725,469; 704 MB/s |     647,186; N.A. |
| University_of_Canterbury_The_Calgary_Corpus.tar                       |    3,265,536 |  1,164,397; 368.2 MB/s |   1,241,281; 1055.0 MB/s |  1,316,869; 576 MB/s |   1,017,658; N.A. |
| The_Complete_Sherlock_Holmes_-_Doyle_Arthur_Conan.txt                 |    3,714,387 |  1,422,283; 302.6 MB/s |   1,539,507;  915.7 MB/s |  1,348,088; 640 MB/s |   1,285,462; N.A. |
| The_Project_Gutenberg_EBook_of_The_King_James_Bible_kjv10.txt         |    4,445,260 |  1,512,215; 322.2 MB/s |   1,605,032;  952.2 MB/s |  1,455,516; 704 MB/s |   1,320,100; N.A. |
| Ian_Fleming_-_The_James_Bond_Anthology_(complete_collection).epub.txt |    5,245,293 |  2,079,270; 298.7 MB/s |   2,256,844;  899.4 MB/s |  1,952,804; 640 MB/s |   1,869,849; N.A. |
| Complete_Works_of_William_Shakespeare.txt                             |   10,455,117 |  3,851,884; 313.7 MB/s |   4,200,287;  908.9 MB/s |  3,755,784; 512 MB/s |   3,378,656; N.A. |
| Complete_Works_of_Fyodor_Dostoyevsky.txt                              |   13,713,275 |  5,127,549; 303.2 MB/s |   5,569,991;  906.5 MB/s |  4,668,918; 448 MB/s |   4,617,360; N.A. |
| The_Book_of_The_Thousand_Nights_and_a_Night.txt                       |   14,613,183 |  5,855,516; 306.2 MB/s |   6,223,909;  914.7 MB/s |  5,375,030; 448 MB/s |   5,198,949; N.A. |
| Dune_Complete_17_Ebooks.tar                                           |   16,973,312 |  6,660,569; 299.5 MB/s |   7,290,214;  894.2 MB/s |  5,990,162; 384 MB/s |   6,086,933; N.A. |
| Agatha_Christie_89-ebooks_TXT.tar                                     |   33,258,496 | 12,404,504; 305.5 MB/s |  13,365,090;  909.2 MB/s | 11,006,648; 320 MB/s |  11,173,195; N.A. |
| Encyclopedia_of_Language_and_Linguistics.txt                          |   59,416,161 | 19,435,030; 358.7 MB/s |  21,118,907; 1038.8 MB/s | 19,187,697; 320 MB/s |  17,546,530; N.A. |
| Stephen_King_67-books.tar                                             |   61,382,144 | 24,402,931; 299.2 MB/s |  26,310,486;  894.3 MB/s | 21,115,141; 256 MB/s |  21,854,632; N.A. |
| enwik8                                                                |  100,000,000 | 39,573,323; 325.6 MB/s |  42,283,793;  936.9 MB/s | 35,453,517; 256 MB/s |  35,102,891; N.A. |
| New_Shorter_Oxford_English_Dictionary_fifth_edition.tar               |  132,728,832 | 28,968,421; 460.7 MB/s |  30,133,137; 1169.4 MB/s | 30,773,086; 384 MB/s |  25,418,601; N.A. |
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Note:
LZ4 uses 32bit code;
ZSTD uses 64bit code;
Akuuma uses 128bit code.

As always the C source code is included.

Drawing some quick conclusions:
- ZSTD almost trifectized GZIP - surely in compression/decompression speed department and arguably in compression ratio department;
- GZIP is a thing of the past;
- LZ4 actually trumps ZSTD in the most wanted segment - where balance between decompression speed and compression ratio matters most;
- Akuuma is slow on non-optimized for XMM CPUs, it requires huge caches.
*/

// Nakamichi_Shifune.c, is like 'Mustang/Crazyhorse' but uses unused 1111b tag and step 3 (instead of 2).
// Nakamichi_Mustang.c, using 16B/4KB/1MB/256MB or (8-4)bit/(16-4)bit/(24-4)bit/(32-4)bit windows with 1/2/3/4 bytes long offsets. Superbly optimized both ratiowise&speedwise for English texts.
// Nakamichi_Loonette_Hoshimikou.c, using 16B/4KB/1MB/256MB or (8-4)bit/(16-4)bit/(24-4)bit/(32-4)bit windows with 1/2/3/4 bytes long offsets.
// Try it: Reduce the 256MB to, say, 4MB or 8MB to boost it.
// Tweaked: ML 4-8-12-16 are in use.
// Nakamichi_Loonette_Hoshimi.c, using 4KB/1MB/256MB or (16-4)bit/(24-4)bit/(32-4)bit windows with 2/3/4 bytes long offsets.
// Nakamichi_Hayabusa.c, using 32B/8KB/2MB or (8-3)bit/(16-3)bit/(24-3)bit windows with 1/2/3 bytes long offsets. Only 8 and 4 matchlengths.
// Nakamichi_Tengu.c, using 16B/4KB/1MB or (8-4)bit/(16-4)bit/(24-4)bit windows with 1/2/3 bytes long offsets.

// 'Shifune' a.k.a. Totenschiff a.k.a. Coffinship/Deathship
// 関船 - sekibune/kanfune :type of fast military boat used from the Warring states period until the Edo period (Archaism)
// 棺 kan - coffin
// 船 fune - ship; boat; watercraft; vessel; steamship
// 死 shi -  death
// shini 【死に】:
/*
        noun:
            dying; death;
        noun used as a prefix:
            dead; useless
        prefix:
            damned
*/

// shinibana: 【死に花】 noun: a blooming cut flower; glorious death
// shinigami: 【死神】 noun: god of death; Death
// shikai: 【死海】 noun: Dead Sea
// shima: 【死魔】 noun: demon of death —Buddhist term.
// Source: http://tangorin.com

// Dedicated to B.Traven a.k.a. Hermann-Albert-Otto-Maksymilian-Feige, (parents: Hermine Wienecke and Adolf Feige).

// It was also confirmed that Otto Feige’s father had worked in a factory that made coal brickettes for use as fuel – a likely source for the name of Ret Marut’s anarchist paper, ‘Der Ziegelbrenner’ – i.e. ‘The Brickburner’ or ‘Brickmaker’. The Feige family also related that shortly after Otto had written from London telling of his troubles with the authorities, his mother had received a visit from the police enquiring if she had a son named Otto. They were presumably checking out Marut/Feige's story on behalf of the London and US authorities. Scared that this was in relation to Otto's radical politics and his being held in London, she denied his existence for fear of bringing trouble on the family. The family later found her sobbing with fear and shame. That was the last the family heard of Otto until Wyatt and his BBC film crew appeared 50 years later.

/*
   .::;.,.;:,.,,;,;,;,;,::;:,:;.:.:.,.,...:.. ..: ....:.....,...:.......    . .     . ... ....... . . . ... . . . .   . . .   .     .    ...... . .           .@@     .    .... . ... . .  ;    ..  . .   .     ... . . .   .             . ..... .     .     .                                             
               ...     .       .                                                                                                                              :@C                          :                                                                                                                
  :.:.,,;;;;;,;;;,;;;,;;;;;;;,;;;,;;;,;,,,,:,:,:,,;:,:,.,:,...:,..:.:.. ... . . ....... ........................... .   . .     . . ....:...... ..  . ...     8@I    .. ... . ......:...:..;; ............. ..... ... . .     . . . . . ........:..............                                             
. .,,,:;,;,,,;;;,;;;,;,;,;;;;;;;,,;;,,,;:,:;,,.,.,,,:,:::,::.:.:..:............: ................. ..:.... ... ... . . ... . . ....:.:........ ...   .     .  i@Q      . ......:.:.... ... ;;. ..:.:.......... .   . .   ... . . . . . ..........:...:.......... . . .   .                                  
. ,,;.,:,,;,;,;;;;;;;;;;;;;;;;;;;,;;;,;,;,;;;,,:,:,,,:;:,,,.,,,.,:,...,...:...,........ ......:.. ..: . ... . . ... . . . .   ....::,...... ..... ..     ...   @@   .   ........:...:..... ;; ..:...:.... .....     . . . . . . . ....... ....:.:.:.. ....... . ..... . . ...                               
  .,:,:,:;,,,;,;;;;;;;;;;;;;;;;;;;;;,;;,,;,;,,,,,;,,:;:,,,:,:,:::,::.:.:.,.,.:.,...:.:...:.... :........ .   ..... . . ... . . ..:.:..............     . .   : B@    . . ... ..:::.:.:.... ;;....,::.:...:.... ...   . ..... ..... ..:.... ..:.:.....:...... . . . . .   . . .                              
. .:::,,,,;,;;;;;;;;;;;,;;;;;;;;;;;;;;;,;,,:,,;:,:,,,:;,,,,:,:,:,.,.,::.,.:::.:.:...:.,.:.............:.. . ... . . . . .   ..... .............     .  ..    . 8@   . ... ......:...:.:.:. ,;...::,::.:.:.:........ . . . ..........:.......:.....:....................     . . . .                         
. :;:::,,,,;;;;;,;,;;;,;,;;;;;;;;;;;;;;;,;,;:;,,:,,,,,,,,,,,:,,;::,,:,...,.:::.:.....:.:.:.......... ..... ..... ... . .   . . . ....... .....   ....... . . : E@      . .   . ......:.:.. ;;....,:,.,.,.,.:.. ....... ....:.:.:.:.:.......:.:.:........................ ....... .                          
. ,:,:;,,,;,;,;,;;;;;;;,;,;;;,;,;,;,;,;:;,;,,,,,;:,,,:,:,:,:,::,,:,,:.:.:.:...:...:.,.:...........:.. . ..... . .   . . . ........... . ...    .:.. ..... . .. O@            ,;.....,::.,..:;.....,.:.:.:.................:.:.:.:.:...:....:,.:.......... ... . ....:.. . ... ... ..                        
  .,:,,;,;,;,;,;;;,;,,,;,;,;,;,;,,,,:;,,,,,;,;,,:,,,,,:,,,:,:,:,:,:,.:...:.:.:.....:.:.............. . ... . . .     . . ... . ... .......  .,.... ...   .     C@. .,;;iilvVlyL, :.:.::,.:.:i....,.,.:.:.:.............:...:...,::.....:...:.:...:.................. .     . .  ..       .                  
. ::,,,:,,;,;,,:;,,,;,,,,,;,,,;,;,,:;:,,;,,,,,,:;:,,,,,,,:,.,:,.,.:.:.....................:................ . ... .   ..... . . .......   .:,::..         ,,i;,E@Y;ivi;;;:..  .. .....:.:.. i.. :.::,.:::.... ....... ... :.....:...:...,.....:.,.:.:.:...... . ... . . . . . . . .   .                     
  .,:,:,,;,,,;,,:;,;:,:,:;,,,,:,,;:,,;,,:,,,.,:,:,,,:,.,,,:;::.,::...:.....:.....:........ ..... ... . ..... . ........... . .   ...    .:...... ;i;ivvYllvil; G@i            .......:.. . .;: ..:.:.,........ ..: ..... ............:...:.:.,...:.:.:...:.. ... . . .   .   .  ..                          
  :.:.,,,,;:,,;,,,,:,:,,,,;:,:,.:,;.,,:.,:,:::,.,:,,,,,:,,,:,,,.:...:...:.:...:...:...... ..... . ... ....... ..........    . ....   ..........  Yvi;;,..   :  c@v    . . ... ........,.... ;,. ....:.. ..............................:.:.:.:...........:.... ... ....      . ...                           
  .:.,.,:,:,,,,;,;,,,,,,,,,,:,,,.:.,.,::.,:,:,:,,,:,:;,::,:,:,.:.,::.,.,...:.:.:.:.............. . . . . . . . ....  . .   . ...   . . ......... ;     ....:,  X@V   .   . . ..: .........  ,; . ..... ....... ..... . . ..........:.:.:.:.:.....:.:.................. . .   ..:.... .                      
  ..:.:,,:,:,,,:,,;,:.,:,,,.,,,:,.,:,,,.,:,.,,::,:,.:.,.,.,:,.,.:::...,.:.::......... ......... ...  .. .   .   . .   .     .     . ... . . ... .: .... ...,,  j@X  ..    ....:,......:.....:;. ..:........ ... ..... . ... . ......:.,.:...............:...:.. . . . .   .   . . ..                        
  ...:.,:,::.,:,:,:,:,,::,,,,;,,.,:,:,:,::.,:,.,:,,,.;:,:,:,,,.:.:.:.:.:,:::............ .   ... .   . . . . . . . .     .     .   . . ... ......, ....... ;.  y@3   .  .. . .., ............;.  ..... ... ... ...   .. .. ... . ....:.:.. ......... :.:...:...... .     . .   .   ...                      
. ..::,:,,,,;,,,;.,:,,,...::,,,:,:,:,:,:,:,:,.,.:.,::.,:,.,,,:,.:.:.:.:.,.. ......... . . .   ... . .   .     .                   . .  .. ... ..:.. ..:....,   l@S    . . . . ., .. ..... . .;:   .   ... . .   . . . . . ... ..............:...........:............   . . . . ..  .                       
   ..:.:.,.,,,,,:::,::.;.,:,:::,::.,:,:,:,.:::.:.,:,.:.::::,.,.............. . . . ..... ...   . ..  . . .   .                 .     .          .. . . ....;   y@E     .   .  ., ......... . ,.  . . . .   . ..... . . . ... ...........:..:.......:.....:........ ....... . . . .                          
  ..,.,:,,,:,,,:,:,:,:,,::,:,.:.:.,.:.,.,:,,,.,.:.,:,:::,:,::.:...:........ . . . . ... . .   . . .                         .                 . :. .... . :,.  l@E      ;,  .  :. . ......   ..    .. .   .   ..... . .   . . ..:.........:...............:............ ... . .  ..                         
  .:...,:;:,:;,,.,,,,,:,.:.,..:,.:.:.....:.:.:...::,:::,::...:.:.:.... ...               . . . . .                                         ... .,. .      ::   v@8       ,.    , ..  . . .   ,.    . . .   .   .   .   . . . ..... . ......:...... ........:.:.... ..... ... . . .                          
. ..:.,:,:,,,,,:,.,,,:,:,,::,:,.:::.:.:.:.... ........:.:.:.,::.:.... . ...   .               .                                             .  .,       . ,    ;@3             ..   . .      ..     . ... . .   .   .   ...  ..   . ....... ..... . ....:.... ... . ... ..... . .   .                       
  ...:.,:,,,:::,.::,,;:,,,.,.:.,.,...,::.:.:.:.:.:.........:.......... . .   .                                                                 ..         :    F@8j            ..  .   .     ..        . .   .         . .   . ... ... . . ..... ......... . . ... . ..... ..                               
. ..::::::,:,,,:,:,:,:,,,.:::.:.:::.:.:.:.:..::.:...... ..... ... .       .                                                                     .              V@@@l :i.3;     .,             .                   . . ..... . . . ..... ....:.. ....:.:.... ......... . ... . .                             
  .....,:;:,,,::.;:::,:,:,:,.:.:.:.:.......,.:.... ..... ... .     . .                                                                                    ,;ijycB@@@@@@@@Q:     :  .         ..                    . . . .    .. ....:.......:.......:...: ... ......... . . .                              
  ..:.,::.::,,,.::::,:,.,.:.,.:.:.:.:.. .....                                                                                                           v@@@@@@@BB@@@@@U  JKv   .                                       .     ... ..........:.,.......... . . ..... ...     . .                             
   ....,.,:::,:,,;,,:,.:.:.....:.:................ . ...     .                                      .                                                   Y.y:QBV6@@Sv F8i  ;@@i  ,.                                         . .   . ..... ......:...:.........:........ .   . . .                            
   ...::,::.;::,,.,,,:,:,.:.....,.:................ .   . . .                                       .                                                  Y@ ,,O   @@v  .F    :;         .                      ;                  .......  ........ ....:.:.:.:........   . . . . .                           
   ..:.:.,:,.:::.:.,.:.,.:...:.:.:.............. . ... . . .                                                                                            , i@v   B@U   ii                       .            @Q                 . . .       ....:.....:.:.:.:.......... ... . . .                            
    ..:..:,.:.:.,.:.:::.:.,.,.:.:.....:.... ....... ... .                                                                                                 .U;   @@j    v.                             . .  UB                   .      .   .. ....:...:.:.........: ... ... . . .                           
   ..:.:.....,.:.:.:.:.:.,:,.....: ......... . ... ..  .                                                                                                  ,,    @@V     ;                                 iB                     .   ;@@@; ......,...,...:........ . . ..:.. . .                            
    ..:.,.::,.:.:.:.....:.:.................. . ... .                                                                                                    .;     @@G,,,6 .                                 8S                      .   @@@l  . :...:.......:.... .       ... .                               
    .:.:.:.,.:.,.............:...... ... ..... . .   .                                                                                               @i  : vU.:,@@8  i@C .                               @FD                       .  B6     ......:............ ..    .  .... . .                          
    ..:.:.:.:.:.:.....:........ . . . . . . .   . .                                                                                       @l         @@   .@B   @@B                                     @;0@;                     ..  @J    ... ....:.:........ ... .   . . . .   .                         
    .:.:::.:.:.... ... ....... . . . .   ...       .                                                                                      Z@  .    ;ii@@, .     B@@                                    BX  .                          @8   . . ......:.. ... ... .   . ..    .                              
    . ..:................ ..... . ...     . .   .                                                                                         $@i ;      O@@y       B@@                                   @S                        . .   B@  , .   ..:........ . ..    . . . .                                 
     ............:.... ..... . . .   . .                                                                                                  ;@8       .@BQl       Q@@                                  68                          .    O@  :    . .   ..... . . .   ...                                      
    . ....:...... . ... ... . . ..                                                                                                         B@          ,@       Z@@                                 l8                            .   K@      . . ... ..... .         .                                     
     ..........:..  .. . . . .                                                                                                              Gl          $l      S@@         .                      ;@:                             .  y@;      . ... . . .     . .                                          
      . ... ... . .     . . . . . .                            ;.                                                                           .6          i8      S@@                               .@v                             .   i@l       .   . . .                                                   
         .......   .     . .   .                     .iI8@@@@@@@@@@@V;  G,   l    .                                                          6;          K3     K@@                               Et                                  ;@l                .                                                  
        .     ...   .   . . .                       @@@@@@B8@BBBBEBB@BSJSLi.i2  O@@@@@@E2Uv.                      .@;                        vE           @.    t@@                              G6                                   .@C                                                                   
             .   .                                 i@B88Q8E8E8EQQSQ8E@@@@@@@@@@@@8EBE@8@@@@@@@@8Fyi.               3                          8,          cQ    L@@.                            V@                                     @8                                                                   
        .                                         .@8SQ$SD0$GIGGDI$ZS32EB8BB88BBBEEQEE88B8BB@B@@@@@@@@8Fv:         6                          iO           Si   V@@:                           ;@                                      @@               .                                                   
                                                  OBXO3IO$KIKKGGOZ2I31LZEB8B8B88BBBB8BB88BBB88Q8E8E8BB@@@@@@8Ci,  ,2                           8           .8   ;@@;                           B.                                      B@            .                                                      
                                                   iF233OI0C113J2CMt1OMFZ6QQ6D$3QB@@D2CXtyICJUCM$G8E8E8888EQ8@@@@SQIv                          iX           Il  i@@;                          Q;                                       E@ii18@l                                                             
                                                    lKC2F222ULyXUjVcUG3FUFc2tJcII. ;QXLcYUVVyvV3F2IG$EZ60GG8QQEQZ8QS@@@8Ui                      3            8  ;@@;                         ZV                 ij0@@@@@@@@@@S    F@@@@@@@@@3lj                                                             
                                                     vtXUV2FFXccJXXllvycFXKX1X2SO   @ICF3M3cXVc3FcUjMUVliY6GO0QQ0S3G8FCBB8@8I1v.                lv           yX ,@@i                        KM                 @@@@@@@@@@B@@@BI    ViC.t@Y .  @                                                             
                                                     ;iJ$8B@B@@@@@B@BEZI0I11F3KLttjE$KJUcXJtUlvjcVJ1LVUlvX0KJX20K0$DCIS8O328SQ88BB2Y;            $            E  @@L                       lQ                  D@BE88B8B8B8B8$t    v;  X@i  ;                                                               
                                                      i0@BBB88B8B8B8BB@@@B@8B8E0SDEO1VVliiivyVUXFcXtFccVVcCJJcLc6U0Xj3SZS$SECCS2ZESJLQBIV,       vi           il.@@V                  .   ;B.                  X@Q888E8888BBBI3    .;  ;@j   Y,                                                             
                                                       B@BE88888E8E8EBE8E8888BBBB@BB8888Q$tXllillLVVYLcjyXttcciLQVZVl3KFjFIQ3X2OV1Cy;yylZ@@DVi    i            F;@@X                      @                    V@8Q8888888E8SG$    S   ,@D   ll                                                             
                                                        @@BE8EEEBE8E8E8Q8Q8E8E8E8EEQ88BB@B@@BEE$FXclvvvvlvLyVlVEBSFUiFYiYiilyiXtLljyLviLUISGK0Q86yK,            Vv@F                U    @;                    i@88Q8Q8E8E8B6l1    ;vy ;@@  Bi.                                                             
                                                        i@BBE8E888E8E8E8Q8E8EEE8E8QEQ8E8E8888BB@ESGBQQO2FcVli,FIyIMOXjlXJOyUJjVUcY;1Icy20YOIlGtXGOQ$yv:         l;@E           . .      Ei                     ;@@E8E8E8QEEBZyy     Uy  @@                                                                  
                                                         $@BBE888E8E888Q8E8E8E8Q8QEQEQ8EEQ8QEEEQEQEQEBBBBBBB8CG3yV1X2tSZQOKKGXLC6U;t0SjjVy2lijO23JtyjyVFIE       ;;@            ..     cI              .       ,@@@BBE8E8Q8BZvt         @@                                                                  
                                                          @@8888E8E8E8E8E8E8E8E8E8Q8E8QEQEQ8QEQQQEEEE8E8E86$QBEEQQ6EZQZ030$XlUJMLXVG2tYUiiijYt3JU1XylvVQO8V      M;$            .     VM              i@@@i    ,BlcOO88BBB8@6yc         S@                                                                  
                                                          i@B88E8Q8E8Q8E8E8EEE8E888E8E8Q8EEQ8QEQ8QEQQQQ6EESKQQQDQQEE8QQQQSS2GIFy;iKS6ODXiVKLSFYOGJUivij6FCD      @v;            l    ,B      ,         t@v      @2SEZOBSE@@@BFS         V@                                                                  
                                                           8@BE88EE8E8EEQ8EEQEQBEZSS$EE8Q8QEQEQEQEDEDQDQDQQQQQZEQQDQZDGD6E$88EEEOMFXltvvv;;ilXcES3MYjvVGFtCK     @F;            V    B                  @       @@DQD$D$Xiy@@y2         v@                                                                  
                                                           .@88E8Q888QEQEQ8E88BScUS00Z8Q8Q8QEQEQEQE666QDQDESESQQEQQQQ2S6Q6QDQD86ZDB8Q$0GIvii,;U2GKKFGytCUVMUl    ZO,         ;      @           i      ;@i .Vic  IB@@B@0y   KCF.        ;@,   8                                                             
                                                            @BE8E888EEE8E8Q8EBEtyJZEQ88BE8E8QEQEQEQE6QQEQQSQD6QQZDS8SOQQ6QZDSQZGI06EZQQ8B@BB2F$JUUlYUX2KlcVlYC   F@C;         ,    D.       iMjjI@Q@8B@@@BB@@@@X   iIU0@@BE1Y$Fi         @i   @v                                                            
                                                           @@88E8E8Q8E8E8EEQBBIt2EBQ88B$0E8Q8EEQEQE8@8BBBQEEEQEDQQQSDDQ6Q6DZDQZ20OQZOSQDQQESQ88EQD1iiyvVJKGliyIi Y@8L             J;        @@@@@B@B$IQDE2ISE6B@;     ,c$y3@@Ucj         @1   .3                                                            
                                                          B@EB888888E8E8E8Q8DGFZ@BD8BEUit88EQEQEQ@iVUO8yV88808888EQ8QESEDEDQ6EGG0$6ZCS6E8QS02QQQDEZ$IIclvvli;iULj$It;,           Li        t@SIBEQIEE8B8ISS@@BDB@@ v ii;.,:;@I ,i        @8  , v,        ;$                                                 
                                                        j@@@ J@E8E888Q8QEE8DM0MF8yFBB3XUIE8QEQ8DEBM YB;j;Y@lyv8ZlBZ0EIB8BEE8B80I$ODG00DQ8Z63ISQZQ$ZFCSDIOtv;VjVvvF6Dy:l         ,2         @@i M@BSQ8D@B0ISD6GQQ@@i  iL  jV VF;;;        @@i   lC         i@                                                
                                                       @@@BBI388Q8E888E8EE8E0Z6$FyO@Q3IQS8EEQEQQQB3;@Si@vC8iSyEL;@L;;3Q;LVBYlVO$G$$0I66QSQ0SZQDQDS6SGG3C3GViI$2F;iyFSVF         t         $E@j :McG$FF1LICVcVJSZZ8@8O@$I$BQFK8B8EGJV.   .Q@     U.         D                                                
                                                        ;@@EB88QEE888E8E8QE8B6ZS2IItFOEZEE8QEQQ6QEKX@OV2iBt.:E@iG@; 8@1 iEE VVS$0G0CIISSZ$S$6QQQE$E0C11M$SFlcIMQyiiVF2$C       j         8BKQG ;XXZ03333DO236$ZI$SOQ@EBBBE8B8DDQ8B@$;vFyi2@      c         Ui                                               
                                                          @@8E6EQ8E8E8E8EQ6EQQQ8SSSQJ1SS66EEQQDQQEQ8E8ICQBtDv06;BS,,S@i D@F;JMQQGG0GO0SQSSI03O3S$20GFFX2F2Gji0XQSXiVycyGl     @   .ilJMQ@B$GjB@@SQ6GSZSZSIMD$ZS$I03SQZZ8Q6$DSS$0$66X  ;iivB      D          @                                               
                                                          I@@BEQEDDQ8E8EE6DSD$Q6ED6$DO6SSZ8QQEEQEQEQQQ888QB88QQS8$6jVBvy,@y,X0QDS36$ZSQQZ$SS03GCI2OI3K1XcvyIIcOEEcV1M3YXB   ,@@Q@@@@@B866S$$Cc8ZZ3$CSOSG030$SZSZSGO0D$$$$SQS0$ZI$S8Xic08i.jV,    cU     :   8.                                              
                                                          @@@@@BEQ$8E8Q8EDSQZZ$0GZSD$Z$D6Q6QQQ$DQQQQDE6ZZQQQ6DSD$EDEEDQB0Q$clMZQ$$SZSQSS$03OSSIS$0ZQ6D6E6I12OXFZtiY03$20@,  C@SE6ZSQQD0$0SOSSi$DOOC3SS$$2SGS$Q$Q666Q6Q$SGDGOO0$G2$DMi@@$i,iSK ;  ;1     :   ;Z                                              
                                                               @BQQD8QEE8ZQZQSSGSO00$0S6Z0SDDSD66$ZZD6D0$ZQ6Q6QS6$6SQSQ686Q8BQQZSO6ZZSSO00I3Q$D6Z0$ZD0IIIjM2GFMK1vUI0YYI0@ ;@D0SDODDSS$3QSG36VlE$2IKZ$SG6SSSZ$SSQ6QSDIOQBB;JBEQISSEOi;  , ;yOLi   i;         B                                              
                                                               E@ESZQEQ8QDDESD0OGOSSSZSSGZQ6OZZQZDSSO6ZDSQZD6QDQ6Q6QZDSD6D6Q6DZDGZ33ZSIIM6OSGZZQDIcK1KFtYlVXXVL03jMSOUYO;X@Y G3DMtXtK3MZED3OSE;G6GO0ODSSSDZQ66IQD6SDG63B@@j2QGSQS6QDl;VKv.;il ;    l   I;    jl                                             
                                                               i@8E68EEQEQDQQSO3I$Z0QSO3SQEDQD$SQ6D$ZZQSD6DSDZSS6ZQ6QQ66Z$ZSDSQZQDSG6G0FGOO2O$DQQCV21S81jlX2SLtOS;c$SOj1jicl.MByU3$IIGXJMXCFF3il8ZQMIQQQESQEEQKZ8QQQQQDZ8QcLOicBQ68DciB@@i,ilUc;;; ;.   @.    @                                             
                                                                @BQQE8EEQEZQSDS$SZ$$00O$06QEQEZD6E6SSZZDDQSDZZ$DSZ$Z$DZ6G$0$06SD$ZSDS6QSO023c1KCI6c0BKV;lC$S6$GM0vl3OIyUiilyliylX$QB@@@8EESE6ESSQ8Q$StQ8Q6QEZSFS0ZSDSQ8DQBCUFOS6308BC;S@@@;;;S@@8Stli.  ;  .. XL                                            
                                                                @@Q68E8EE6D6S6QSZ6$$S$D00$EDE6QSD6QZD06DQ66SD$Z$SSZGS6E$SSO3$Z$GSSZSSG$$$3Jlvii;lXYD0 ;, ..  2IiLVKSSEDtiill;,iXXXVJXYI$B@@E@@KMBDSSD0ZDQI2J8Q00OZQDDC8DSEGUXcE$SFVXF,t@@@Q,;i@@@@@@@@80y,;i; vY                                            
                                                                F@QQE888QQSQSZZSODO$S$SZ$QSDZ6$666SQ6DSDZDSQSDSS$ZSSZD$$SSI$OSG0SZO6SGCO$$$S33IMlvvL; ;v ;,    ;;;;, iVY;Yji ;JGMCKvc2U;;ljtS8$1E$GEZDD$28CJB8M2M$QEZXEE263cUvBB@8,;lViivGK;;,2@@@@@@@@@@@@@@DIXiF                                          
                                                                .@BQ8B@B86EQZDZ3S6ZG$SS$QDQDQZQS6SDS6S6ZQZZ06ZDSZS6$6QQ6ZO00$GS0Z$$Z6G0IZG00QSQ86UQS    ,    ,0DC.  , .  ;l;; iS, ;,,;Ui   .;;yVIGD6$FQS1YlYQMISQ$QD03EED$DJtiB8B8;;ili;;;iii;i8OitEDc.iX ;;tD X,.,                                         
                                                                 @@B2UyyBB8QZESOD8Z$$DZE66$SZZS6S6ZZOSSS6666SZ0ZSZS$OSDQDSOSOO$S0S06$Z0OO$O$$SS8tISM;;,i;;   ,Scll; ;,,;  ;@D  ;   .  .;..       Q3FMytEQDY08$GSSE6OMXZBQ$SMFl888@i;iyviiiiliv;yU    ;       i    .                                         
                                                                 Q@B01XJBQ8E88ES88D0ZQ8EQ6QSD6QZQSZSS06$ZSZ$SO0O6$$G$GSS6$$0S06O$GS$$OSO$I0G$IS$20Qcv. :;:   i@i  v; .  , ;G;       .  i;.. . , :$CyXtM2GD0$QSD0SZ$F1088B68KCiBBBBi.iiviivvllviitJLi  $;Y    i                                              
                                                                 v@8ED$BGlV1ccJcVIQ8Q88BEQDQ6QQEQ8E8E88888QEDD6DZ$$GSZO$0$ISSS$ZSSZSO03$3$OOG03ZO$Si;i;,;;:  i@E  c1yvi;. .i.:   .  .: V;;,..:i:;0SVLXjcVjcccCF$GQ88E@@@@@@0Ki8@B@IXcLli;iiviiiiiB@@8@Q$t,;l v     .                                        
                                                                  @BQFtQ8YJJLUJLylLUFFCK220DQ3G$2KI3OSZ6S0DEBBBBB8BQBBB8BEEQ8QQDEQ0G60SOSO$OS$SSSO2,ililj,.  v0iv.@$8@LV1icVi   ;y;...,;,.,..i;  lELUXXJXc1UyVUVYVFM3GGO8@@8El@@@8U30O6S3KJUcliii;6D@@3@@@v .B ;:  I     yy                                 
                                                                  B@EEEBBXcXLJctVjlyiivi;iiYlYyUXKUi;i;;,,,i;i;iiVyyL1J3F33GD8Q88B8BBB88EEDEQE6QS82y;UVvGFvv YE6; :;;Vi;lv0BViLi;i;;,..;iv;  vO   EGjXXXJFFXVUc1UVyLc1, .,iltUB@@iiUX1yXXMX1M2C2cVjQ8E088@G  @@@Et @   lcv.                                 
                                                                  X@BUXQ@tJcUUVcUVYyVlVVylviiii;i;i;iiliiiiiii;;;;;:. ..:,;.,;;;iilUXcCC33$0QEB8@8yljUQD$6QDKZEQi,:,i :.,lS8i.CtVci;v;;;ycv   @BX:tQCMGM2F$KcLX$S3cLJM; .:, ;;i;VYt13F1MScXYjXXjUVLcJjjUKICl.6B@Il8@B                                       
                                                                  .@B8EB@0UXUUUtycVVVVlyVclviiiiviiiiiivvllivviiiiii;iiiii;;;i;;,,:;:,;;:,,;,;;ilvilijiYiXS0@EQcK0DQBDty;ccEi ci;l;VliiVVXU;..iQ@L.v;;vUt2SEGMK6cFXXX6i.;;;;;;;,iyiiiliVctM2CSUtLUVUJv.vVvjUliLI1vcB8;                                      
                                                                   @@QYj86LJJcXcLjjyVYVyVvllliivvivilvlvlvliiiii;;i;;ilvviiilvi;i;ii;ii;iii;;;;;i;;:.   . .:l;l;MYi;18IIBUFSliKl;Vyi;v;LVUMi;i:;vi;li   ..,;iiL3OKOIXIU.,,i;i;;:iUY;ii,.;iyilVLMMMCISt;lVyUil .;Yi,ivVJYi;;:     ;.v:                       
                                                                   6@B0DBELJLUUXLUyjyVYYvvlyylvYllivvvlliliiii;i;;;i;iivii;i;iii;iiviiii;iii;ivllvivii.,;;..,...;l:;.iliviUUiJJ0cIItj2lLljci.i,vii;yj;;;i;;.,;..lXvlltU,.;;;;;;,ijvivv;;iUli,i;lycvVVyilvjiyv..,vL.;i ii;;;yi$Y  ;iliiv                     
                                                                   ;@BFVB8VjJVUUcVVYylyvvivvviiiliiiiiiiiii;;;i;;i;;;,;;;;;,,;,,;;;,,;;;i;i;i;ivi;;ivi;;iii;i;i,;;,;illi;lXVYUcKUiiicjFJXVtVii;ii;;it;iilvi;;ii;;,, .:;;;,;;;;;;iiiiiii.,VXviillLUiiyiylYyVUl,;,ii;;  ii;;;;;vi      .:                     
          .: . .                                                    @@Ft8BYvlyvYyllVlyvlllivviiiiiiiiiii;ii;;;;;;;i;;;;;;;,;,;,;;;;;,;,;;;;;;i;i;;;;;;,,;;:. ;i;i;.iLLvviUlJVI2Vliil3YijViiXUlcXXXlVGi;;iv;;;;;i;iiiiivVli;;;ii;.iii;ii, ;UliiilVUliVvllyYvjl..;;iivi;;iiilVilv;;;;,.;i                     
  :,;.  ..;,,::.,.. ..:....               .:.                       @@Q2Z8FyUJVVUllviiiii;i;i;i;;;;ii;i;;:,,;,,,,.,..:,,;.,,,.,,;;,:;;,,;;;;;;;;i;;;iii,i;;:...,ii .;il;;vcVcVXvVViiKlYVUYvylilYj;;iOYYcjll;i;;;;;;;livvlviii;jl.iVylvli;;iii;;iyvliyvi;iyiLU,.,;;ivi;;ivilliili;ii;.i,                     
. .;,,,,,;:,;, :.;...:..                   . ......  ... .          U@1iXQQ88EE8888DS8QQSZ$$G333MFXtJJjLVylyvliii;,;;;,,...     ..      .:.. ..:.:::,,:,,;,;:..;vi ;V;,, ;tYicy;,i.ill1JlYvvlj;ii;:,ViiyyvVtcjXUvVV:ili;;i;iviVy.iVVlYVJVUVVVlYYlviliv;; vllV; :;iiii;;iii;i;ivviii;,;                      
  ..,,;;i:      ..;;;,;,,:,,,.:                         .           ;@80ZZZZESDZQEEQEEEEQQEQEQ8Q8E8Q888Q8QQQEQQDQDQZQZQDZ0$GI2IK2XFcXUJVLVVllvivviiivviiVliii;;;i;  :   .i;;:;;;,, ;;,iiYv;;vY;;;...llli;;yUliUlily;vVUjyVyyi.YJ iUyyyjYcUliiiJVLLcVlyli..vlV;. ;vi;v,:ii;;;i;ii;;lil                       
   ..,,:;; .   : ..;;;,;,:.::....  :                       .         @QitE$DZDS6DE6QDEZD$$S6$SZS$Z0$$Z$SSS0SSSI00S060SZE6DZQ6EQBE8EEQEQB888BBBB@BBB@BBB@BBB8E8IEGG2XUICFJtX1j1JK2GXUyyvyyyivvyi;ii;;il;ivvyc;ii;,.. :;;ilLXXUXU0cc1FUULVVtv.;iilltXUlyly;..vcV;,,lvvV;.vi;..;ily,.lvi                       
        :.:.;;;,:,,,,.:...  ,.    :..                                8@GOSZDDSSO$$D6DZD6S$Z0S0S$Z$Z0S$S0S00O0G0I0200O300$GOG0IOOS000$3OO$OS0S$$SZ0ZS6SZZDS6SDS$S$0ZGMIOIO2SOCX1FKJUJXUXUJUcycLclLVLlVFilJVcVVXyjy  .;;JVVlVyVU. vlvJlVLLUFt;cjvyjXKKYlUjv,.;XyvvvyFYJ:.vy;.;iill; VK.                       
   ,,;.;. .iyi,;;;        .,.;         .                             l@0vS$6$SGOIZS$$ZG$O03$O0G0I0G$O$I0I$3GIOCOCMC3M32IFMM3C3CCMO23C33GGOIOG030IOI0I$G0ISOS00GSGG001C32FK1CjMUtUJUJLjUcVYVVyVYyYLyyiMyilvlJYylyVi.,. U1VUVXVU.  .,clXjXJF;.lX YLYUMUttUjViyKMcJct2C3V;XUi;lvvvi;,tj                        
    ... . .;;,,;i;;,;.       .. .   .   . ....,.;:                    @6MO$GSOSO00Z$ZS$$SOCISG$GG30GOIII$I$IO332I2ICI3G2CCO3I3$0O3O2OO0GS3020000S$SO0$SOOZDO0G6SSOSG3OS0$CFXjUctLYjULUXUUUUVVjULjyylvcF;lJS3jvyiYY,., lyvlyVyly; :;UVccUFtL,;X;;YyljUVUvli.;XyYlUyYli.ijvi;;;;;.;;C.                        
  :;;;;;;iiivvivivlylYllvlviiiiiiiiiii;iii;;;;;;:;;,,,.,:,,:.... :    @@V1ZS0$$0OZ0SSSOS0Z3OO$0SS0O$I0IO3G3G3I2IMC2I3O3ICIG0I$O$GI3GI$GSO$OOO$O$0Z0SGSO0$DS$$S$Z0S0$SZZ6ZQZ0CGC2cJc1YllylvllvyyLVYlviyFilFMcvlvivji. ,ljllivilVX:;vViyVjYUJFytUlvjvUUcVYiLvvciviiilli. llyiiii;,:yc  . ...                  
. ,,;;iivvvlyilvlvlvlvviviiiiiviiiiiiiiii;vii;;;;;;;;;,,;:;.,..       0@YXQ0ZI$G0IS06$S0SSSODSSGZ600DGS0$GGGG333I2MCCMCMCF3C223M3MIC3300O3O3OGSO$GOI0G0$6$6SSGS$$SSG6$S06SQ$0KCC2F32$6QS0M3FtccLUjyivi3iilvlyilvilyiiiVVVlyvvlyV;;ly;;VlVVcVVUjv;livVyli;iyil;i;;ijylX,iJlv;iilivC                          
, ;;;iilvvivllvliliiivivi;;iiiiiiiiii;;;;;;i;i;;;;;;;;;;;,,,;;,,      i@DFFZ0S$OCIOS$S0S0S$$0OS0SZ20GSG$0$IIKG3I23C22CF2F3KC3IMG3C1GOOG$G03SIGOS00CICGO0SSS6$SSS0DI$$S$6SDSS$OG$CMLyjXUXUUX2CKMKULVVyyFV;i;i;i;iiiiiivivvv;iiiiv;iyv,,;i;;;;icV;.iiiiyllvviilv;i,iYjiv;.VUXLX2MQ@i                          
, ;;iiliiiviviivliiii;iii,;ii;,;i;;;;;;;;;;;;;,:,,,:,:;.: . ..         @QvLS$06G$0SSSS60S$SI$30G022K3O3K3MO3I302I3O2GCI3333M2CI2G3CI0OOGO3OIG3OGSI2C3G006$DS$$S066OISII2S0OGSI03D6Sctjlylvv;;.;,.,i;;,YCiYVljjcjJjVcyilVYlYyUVljVFCXVylVLyVUFcjULXtXCF30SKXFMK13FcMJci. ,;;iJYcVU:::;,.                     
  .;:;iiii;iii;i;;;i;;;,,;;,:,;,:;,;,,.:,::::,,:.;;:.,   ..  :....     8@S3D$S06$6$Z0S0$0$0S0S$0OSOZO0GOM00$G$O$2$OOI$IS0$G$OSGS0Z0S$S0SSSSS$Z$ZZ6$S$6S6S6S03$S$SD6DSZ$66QDQSQQQEE888B8B8B8QS$33IOC21JM81jjJVX30CICC2G3SG0S0GZGZOOGGKMGI$MtMJXyJXKFFCOCCKM1Mt1XJUXt3XViljUlvllvvilvlvvii,;;,:;.;i,          
  ;,;,;;i;;;;;i;;;i;i;;,,;;,,;;ii;i;,.;;;;,,,.;:;,;,;..   . ....       y@liZ6SSSDZQ$606S6S6$S$S$SOSSSSDSDSDSDZDZZSSSDSDZ6SQZQZD6EDQ6D6DD6ZD6DQQSEQQQEQEEEE8E88ES8DQDEDEDQQEEQE8EEE888Q88BB@B@@@@@@@8B88BB$D0IO$GII$33FGG2FKXtXXJjjJcJLtXFcLlVUjyVcUjJXcULyyljJVivlLVyyYlYvviviivvi;;;;i;;;;,:.. ,,.   .     
. .;;;;i;,;i;ii;.,:,,;;;;;;;,,,,..   ...:. ,.;, ..                      @8QEQEDDQQ6QDQDQQEQEQQQQQEDQQEQEQQDEQQDEDQD8E8EEQ888QEB@@B8B8B888B88EZ6@QQB@@BE@@@@@@@B@B@@@@@@@@@@@@@@@@@@@@@@@@@@@B@SDBB8@BB888QM$O3K3tUlylVyjLUjjyVylivlVjJjUcXjUcXcXX1cXUccJccUcyUyVyVyiiviivvvliily;Yvvvi;;,i;;;i,.   ;.    .. 
;.;;iiili,,;;i;,;;;,;;i;;;:,;,,:;ii;;:;;.:iii:;;;;i, .,.;i;   . .       @8Q88E8E8E8Q888Q8QEQEE8QEE8QEQEQEQDSEQQSQQQQ828EZSZOSQ$ijSGF$2SYX2$ViijIl;llil;;iv: vVjYic3lV1XtFUXXVF2XVyjc1cJXjyVlliiiliiviyVllyVyYlVillvYjyVLXUULyivLcXXL1XFFFF2tt1FKtJtXUVjUFUXXXcUjjVUYllyvYYlvivi;l;;;;;;:;;iiliiiiii;..,,;.  
;.;;,;,;iliYiiiiiivlvi;vyl;;;ili.,;iiv;i;,ii,;li;iii;iVy;ii;;i.,;i: .: ,8EODZS0S0$0$$G2O2CFMC2FKXX11tJLUYVyLLUYVyVVUVvlyllvlvli;;i;;viilivliiylviliviyLjlYviili;ilvvvvillylyliivvlilviiiiiiiivlyvlvvilyyYLVLjJcJJtJXVVLXUXcFJXcXJXX2XcXCMKFXjcLJUXXXcULUJUyVVVlYllVUvlivvlvl;;ii;;vi;iiviiiv;;;,.:...,.:... 
; ;il;;;ivlvYliiiiiLUviiiyi;;;i;;;ii;;;ii,;..:;;i;;;i,ii;,i;;:  .,.   i$GcXJFccUJUcjJUcUXUccK1tjLVjYYlVLUyUVyYUUJjUjLjjVcjUVLYLUJUUVVlYlVlYyLvvvllYllvyliivviivivilvylyvllyylvYiiivvlilivilvvlyvllyyYVVyUjVVtcUUcULLXccJcUtUJXtctUXcXUXc1UUUUJccXU1U1cXVUVVVjyVllyyVYllyiivi;;i;;,;;i;;;;;;,.. .. .         
;.,;;;iivlii;i;iiiiliii;;iii;;;vivivvvvii;;iiiiii;il;i;vi,,;..  .  .:.,iVFJJjUJcjJcccUVVYVyLVLLVVLlYYLYjYjVVljVUUjVyiyVVlVVYYYYUVVVylVyllYlyyyvlvyvlvYlliyyYvYYVVjYLyVVVlLVVVtcVlyYYiviiiviiivvyllvlVUYVVUjjVcUXcXUKFUUXUXUUUFJFt1UcU1XFcXtM21cFJcVYyUylyYyLylviiviilliv;iii;;;;;i;;;i;,:;:    ;.           
;.;;iiiiivvyyyjlvyviiiUyi;ivi;iii;ivl;;iviiii;;ii;i;;;;;i:.,;;;...,iyVKVc1XcXctcXXtc1J1cXJUUcUXLcUcUcULUtjVVLyYyYVjYVYVvyYVjjVVYVyVYyVVVLVVYVyjYjyLyVylvllylVVUVLVVYjUUjYlyjUUXlllVlllVylvylylylvlVvYVVUcVXcJUXjXUJtJccJXUU1tcXcMKMCCXXtMFMUccXctFXYjllyVvyVLVVlviyyviiviii;;;i;;;;;;;;.  .::..             
UlVJUJyyvlVXUK2M10MFXLycUyyUcXlllLVVyYiviviiivii;;vi;lVvil;:;i;;:i;..yFFIIF1XXJ1cXcFJXXJUJVjYLYYyLjcjULcVLylyUVjVUUcVLVYyVYVlVyylylyvvilyYyYlylyyYyVlvyUVUyLjVvylVLcyVlllVlv;i;;;iii;vivlvivlLUJllVlvVXXVjUXc1XXLXcUUFJ3Xt1XjUtMFXcFVUJJUtXt1CtCX1UJXXtFUcUlvYvvvYlvii;;;;;;;;,;;;;;;..,:;,: .;;:,;.:.      
;,li;;iiii;;;,iviivii;;;ii;;ii;;;;iv;;i;;;;i,:.,..;:  .,;ii;;;,  .  ;.ivUcXtKXtcFJcXKJtcXUUUJUVVcjVLjyLjLVUVUVUjjYULjyUVYVVyVVjYUyLlYVUVjVjVjlvlUjVvyllivvVlyvlvlyVvllVYULLyjYVUclVllyllyvivyillVYyVLVX1UjLjJUjc1c1LUXUUX11c1XXttJtttXCKFFXULyXKMUXtJVJLjvvilvllUUyllivivvi;;;;;:;;i;,;;,:;.  ::: .         
,,i;;iiviivvllllvivviiiivl;;,i;;,;iiiVyyvlvvii;;;,,;,,,;i. ...:. . ;LVJXvUVLc1cUUciyLUVyVcLUUVYLVVyyyjYVYLjUYLVVVYvyVVYLVVVLVYlVYLjLYUjUjjVUUylyyVjVyVlyvlvvlLLLcUVVYLYYvilyJIFlilyyvvvvii;v;iiivylyvvylljyLXtjJjjLXccjVUjVFVyj1VjlUcUJtUUccUFXjlVVyyyUVllvllYVXULUJYUvvilli;,v;:,;i;:,;;,,,,;.,,.;;     .  
;:iillyyyljVLUliviv;;;;iv;;;;:;;;;; .;;.,.. . ..... ..;li,:;: . . .i;.vvLYKMMttX1ctjcUJyUcUjLVXjULVycLcVVVLVjVVYjYVLLyVjJUUvLULjccUUULcUXUcUUUJcVivYLyVYjVVVlvyllvlVjlLLllvjYli;vvii;;iiiilviiviiilivviiVYUVyyLU1UXjllcjjvjVUJKXXXtXtUXcXXtUccKUcX1XFVllyljLVillyVylyVy;;;;;ilLvi,;;::;;i;;,i;;,,,;;;,;i;;: 
,.iivYyyvlyVii;;i;,;,;,;;,;;,;,,.    ;;;;;,;.;, .,,:;l;;iiVYVL;.,;;llCXYFSCFFF1K1MKFcXX1cXUXX1cUX1XtyctXccUUUJUJjJXUVULcJXUVLXUUUcVJjcLyYjyLyVyyyYlliyilviiivlVyvjLjvyLV;;;;:.,;;;;;;i;;;iiii;vviYlllvllljUUlXjcXXUXVVcXXJUjUFJFXtULUXLLjXJXXFXFXttcjXVyVylVYyllvliiilvviviiii:. , .:,,,;.     ,  .;i;.;;.  
; ;i;;i.,,;;viiiiivvyi;ii;iiilyviiVVyiii;;i;i;iiivVVi;i;;ii;;;;iFvilVUKF1LX11XM1McLtXUJUXXtX1XJJcVVyLjcUyjVYVLjjUvlyjyXULvyivUcUXjVVVUJllYJjVLUVcYYYUUUyyjLjclVVlyyvVllYy;vlVYyylivii;iivviv; ivvyVyYj1yvlVvYUFXcjLVFFXccX2UUX1VUUtJXXXJJFCXXYVVcVUcjYJJXVylLVVllyVVviyvlvvi;i;.i;,,: ;;,.: ,..             
, ,vii;vvvi;;;ii;;;yylllviili;;iii;i;;ii;iiiii;iililvi;;,..,.,;;,.1QCXKUMLUMUCKCXVt2CtUJXFFttMXtUVjcX1tKUUyccXUXLVY1JXcciii;vYllvYvivcJtXUVlvjYVUXcFKXcJyVjyli;ilvivjYyYlviyFXUVliviiVyvvyvl;iiyVyliiLVXcJLUYXIKXXJXjXXK1cJFVcXMc3MOXtXt1KcJccV1FKLJLcJyLXULyVYcLyvyilVLv;ii;iii;;ivvi;..,. .:.  :::        
; ,il;;iiillliiivyVV1UVii;;,ii;;iilyyiii, :.,;:;;  ,i,..;iilXviM3cMMS$S3CJJK2FCMcvLvVVXUULVllyjctLVcjyyvlvLLVjccjYlVJlyUKFMtFXUlUMcJ1Jliylviilljlvv;;iviilllclYUUlllcUtllllvyviii;iiXUl;ii;iiivilvyYUlvvYlvyUccVyvcccjXXcJXLt2XJUjYYjVK1C1XFFJXtFt1tMXUvllyVllVVcyyj1VvYUii;;;;vi;;ii;;:,...: :.     .      
;;yUKCCClivXtGFUVUllii;i;viVjUUl;i,;;ilyivlVUFt2cjUVlXZQ02XM0OXjlycKJSSIG2LI23132GKXUULJylvYUUYylYLcUcliiiyylYjcYLcLjXVyyyXcXti;ivYlYXLLJ11FcJlVVVVllVjV;llVYVVtjLjYvvvi;iyiiiiilVlvliiiiviivyviiiiVXUVUUUXtX32F1UVXUcLXJJKXKGXJJK2O3MLMtFMItFUUYYYVlyVJUVYVllvllLVULl;;ii;ivi;iiviii;.,:,.,.,.        .    
VX02OGZDQZSGI0DSSOGXXjllUcFFI231JjlvYtGG8DXLXcXljcKO32BStMUli;y1$E8t;vGM0IIttFVVK12M2KJXcUXtKcttUU1XtjcUjlLjjjjUyYXXtcXJyllVVlVFFyFJcVUcliliLcJyljC111cJXjvvYVtLlyvll;i;ivyliiiiv;iviilllvllVvlvllyilvlyJLJUVYLlVVUVjctLUtcVcK1323Q00SIIGK11cVVVLyyy1yiiyltFUjLiiiivyvYivivvvi;;i;i;, ,;i;vvyvlyli,...:..   
VVXULX1CCS$IC2cULcUXXXccjVVVlylVVlvVllYliivlvvll;iil;,.,,,;;:ii$Gy:  ,lcISG2XIGIM2MCKCF3KKXFtUUtt2XXLttKXMXXt1FMXMtFtFtKJUjULtXliivviviyjXjcllvliiiljGFtjllttUVlilVVV1XFLYLLviiiivi;;iiivviiiiiyviljjUycXJlVVUvjVUUJUJLUjcUJUXc1XXUyU3tJUcVlyVYVUtjLYjlviYVFcGKUylviiiilylVXtUyVYi;;,:...;..,;;,:,..    .   
YiyiiilllycUUYjUJc1t3KJVJylyliiiilUUULYlyyYvi;;;;ii;,.;:vyliYUii; ;YS6O3SIIOS$S0G33MMCMXXjJXLYUcFXFM3KFFMMF1C3MtC3CFMCFMKFFKCKXyccCXXtXtFF2FKXLUjctXMJyiiiiiv;ii;iUvVcLliiUyi;;;viiivi;;;iiiiivyyvvlVlilULVyFcjjLjK33XUjXCtc1LLVUUXLULjVJVULcctK31MUjX11FXUvUUjLXjyillyly,:;vii;iiviiYcyi,.,v;,.. .,;;;;,   
lilyYylLVXUylVLVjLyjUXVLVyYcylli;;;ii;;;;i;ivv;;illvii;i;iivvv; .;VD61LVVMVKVlVJy2XJlLKtXIMtc1XFMKXcX222CMUcylivjViYVjlcLcctjlvYcM3Fcc1FtXYX31XGllcXVUylVcUcXUycUcK2JLYyivYciiyjVUVyvvycllvLjyvyccUFCJcUVtUVM$XXJtJJX3ttCGM31FXMXXL1Kt36$MXCcXXFXyyXCMXXYyvlVLvVvyvyyvviyUVyi,;i;,;v;i;ii;,;;,.: ..  :.   ;i
*/

// "There it now stands for ever. Black on white.
// I can't get away from it. Ahoy, Yorikke, ahoy, hoy, ho!
// Go to hell now if you wish. What do I care? It's all the same now to me.
// I am part of you now. Where you go I go, where you leave I leave, when you go to the devil I go. Married.
// Vanished from the living. Damned and doomed. Of me there is not left a breath in all the vast world.
// Ahoy, Yorikke! Ahoy, hoy, ho!
// I am not buried in the sea,
// The death ship is now part of me
// So far from sunny New Orleans
// So far from lovely Louisiana."
// /An excerpt from 'THE DEATH SHIP - THE STORY OF AN AMERICAN SAILOR' by B.TRAVEN/

// "Walking home to our good old Yorikke, I could not help thinking of this beautiful ship, with a crew on board that had faces as if they were seeing ghosts by day and by night.
// Compared to that gilded Empress, the Yorikke was an honorable old lady with lavender sachets in her drawers.
// Yorikke did not pretend to anything she was not. She lived up to her looks. Honest to her lowest ribs and to the leaks in her bilge.
// Now, what is this? I find myself falling in love with that old jane.
// All right, I cannot pass by you, Yorikke; I have to tell you I love you. Honest, baby, I love you.
// I have six black finger-nails, and four black and green-blue nails on my toes, which you, honey, gave me when necking you.
// Grate-bars have crushed some of my toes. And each finger-nail has its own painful story to tell.
// My chest, my back, my arms, my legs are covered with scars of burns and scorchings.
// Each scar, when it was being created, caused me pains which I shall surely never forget.
// But every outcry of pain was a love-cry for you, honey.
// You are no hypocrite. Your heart does not bleed tears when you do not feel heart-aches deeply and truly.
// You do not dance on the water if you do not feel like being jolly and kicking chasers in the pants.
// Your heart never lies. It is fine and clean like polished gold. Never mind the rags, honey dear.
// When you laugh, your whole soul and all your body is laughing.
// And when you weep, sweety, then you weep so that even the reefs you pass feel like weeping with you.
// I never want to leave you again, honey. I mean it. Not for all the rich and elegant buckets in the world.
// I love you, my gypsy of the sea!"
// /An excerpt from 'THE DEATH SHIP - THE STORY OF AN AMERICAN SAILOR' by B.TRAVEN/

// Какво ви засягат моите дрипи?
// По тях има радост и много сълзи.
// Какво ви е грижа дали аз ридая?
// За вашата милост аз не пълзя.
// Защо ви е грижа какво ми харесва?
// Тук вече е моят, не вашият свят.
// Небето ви аз не искам да зная,
// добре ми е само в горещия ад.
// За вашата милост не искам да зная.
// И мъртви ли нося или богове,
// не ще ви призная. Какво ви засяга?
// С проклятие никой не ще ви зове.
// Аз плюя на мита за бога възкръснал.
// Не вярвам в небето и Страшния съд.
// Възмездие адско не ще ме уплаши.
// За мен е единствен широкият път,
// Хопла, хей, в открито море, хопла, хопла, хей!
// /Б. Травен, Корабът на Мъртвите/

// 中道 nakamichi [noun]
// English Meaning(s) for 中道
//  1. road through the middle; middle road
// Meanings for each kanji in 中道
// 中 in; inside; middle; mean; center
// 道 road-way; street; district; journey; course; moral; teachings

// 星 hoshi [noun]
// English Meanings:
// 1. star; any light-emitting (or reflecting) heavenly body (except for the sun and the moon)
// 2. suspect (police slang)
// 見 mi [noun, used as a suffix, noun]
// English Meanings:
// 1. looking; viewing
// 公 kou [noun, suffix]
// English Meanings:
// 1. public matter; governmental matter
// 2. prince; duke
// 3. lord; sir
// 4. familiar or derogatory suffix (after a name, etc.)

/*
                                                                                                                                                                                    
                                                                                                                                                                                    
                                                                                                                                                                                    
                                                                                                                                                                                    
                                                                                                                         i                                                          
                                                                                                                :7,,,    :  :v:7F7.                                                 
                                                                                                              L8:i7,,::,.,  .  .i2MF                                                
                                                                                                            r@Gi.ivYvi:7rvMZv,:iYMuii7::,                                           
                                                                                                           L@Gkr.:5kr.JBSSMO7.2B8SS1PJ7.....,                                       
                                                                                                         rB@SFkqJNPuv@Bk ,1NjL22vUL1kGSr, ,.i7                                      
                                                                                                      ,r@BOZ0Fqkkukq2Li  .uSkSuLJ7iiii;vu7. ..:                                     
                                                                                             :, :   Y@@@MMOMPS2FXNXXEO, :P0525J5UULrii..:7ji ,.7,                                   
                                                                                           .7ZE.:v@@@BBGPLJJLuPqq22S0E.:@M0PNXXkXPk20q7ri.   :Jri                                   
                                                                                             i8@@OG0J::5r ,i7FUuvYXZSM.:BMZZ08EZNZ0G0G885Ui,rOLr7L::L                               
                                                                                              rBqN0u:  :EYL7Ji:LvUZqEN0.:BB80qOEG888OZO0S2jivv7:78S0@                               
                                                                                             ,uXF2ri,r,.Lv72kj72qZ88N8M2.LSJrUqZEXNkFNX8OZr i rqYLirB.                              
                                                                                           .irY7i:,      L. ,,vZOOO8MOEqu,.:.uiiUu01SFUSZ@@SL.ruir7i2                               
                                                                                      ,:irrvvkUi   ,,,7, MY     i@BBqNXj7.,i:q7Z0ZXPkkPXur 72: ,.77Sv                               
                                                                               ,:iLukU1L7uEk2r;L:  ,77v7:7u: ,r.  ii:i.ivvirrrNBEq0qNZOFO:  ,,.irkj7U                               
                                                                       ,,.::iuEMM8Pj7vi, :PS5SLYJ, .ii:7ii,   ;2u:  .  :U@:.:0BBE0SZZZJUXr,,vYur7Zr7Y:                              
                                                              ,:Y7:u00O8MGU.,rvvY7r:.r.  .i20Fur:.,,rvrirjJ    .v. .i .X@5r:Mq 0MqqZS..v7  vSGOJPNkv7L                              
                                                          .7uSuukk,vk1UPPZY  i77;: 7ii::7:iuu7r: :, :LL7r2Uv.,   .,.  ,5i:r@7  GMZ0v:,:G1uiiLL;GBLriL,                              
                                                        v25Y7ii7X: 7uJSPPUi  iviri7XS7v1L:7Lu:  :., ,:vJJJuvLj:       ::r.@7   BBZZv:vEOO0JJ27vL:,rLu                               
                                        ,..,         :Lu7rrriii7v,iFkFkqNY, rFUuJ1uqkP1U7J;r.  .,  .:rv1u2U158L    ,: ,:,7B.  .@8G8N77ruu:.UkjL;  v57                               
                           ,   ,  :5N1Yi. ,       ,72Lvr7L27rrLuv :XG5qk8J ,vY5Su2kFqkSSSPvr: :r. .rjvYUYjk5E8Fi   rU  ,7@@.  @B0J5Su.;58MG    ., vu,                               
                         ..   .,7FL:,            iuJrvjk1XqNurL0E, J5FF0M5 iLLuPFS5SFSuu12US7 ,Li:rvYJJPkXPqSuLqr  . .i:2BBi  B2Ji;S7iiuB5        r.                                
                        r.   7Fv:             ,i77;7LFXXqXEqPri2Gi j1j2kBL,YM2uXP2uYU1FLJuuEM7.vS7777iii1kNPXLi:iv.  .iJ1M@  J. iG55i,iBi                                           
                       :.   rP.              JBv:jLjj0qqXNXPqq;Yqi:81u2PM7 ;G5SN1uYJuX5uu1vPMM5XZ7i:  .r750MXr:..: ...rjq@Z iM  ,SkSULS.                                            
                       i   rv      ,,    ,ivMBN.iYk1F00kkPEULX0uNi;Mq2XkM7,SkY5uJF52kXq5kui7uMMZOFii:ij5FUXkOLirur;GkkvY@O  B:    rYqv                                              
                       :  iLi  .,.       LLOMM5iruSF5qPkFPS:iFFSkiiOZ0FqEj2OkF2uYXPkkPkq5Uv:rOM8X0FP0X2. ,.qBOM@X:XG8L:@X   Zk:7, 7M.                                               
                       :: 7rLrLL2Ji..,.Lq:SMMO57UuFXSSNSkPFi:Lj1qL;EOq0PZkPG0kPSSU55P51F5JXvLEGZNkEF0qM:  rr@BB8kXOP5UB     r@@B@Mv                                                 
                       ,u,L015;i:rS5M@BOL0OZO8Uv2P2k5XSX1kULi.7SN1iOOZqNqNqZ00qENPSXP0kqUvj2NGN8qSNE:.ru  ;::iuPqPkSJXB        ,                                                    
                       ,rk8k1X0Y1kuFkuYrEO8O@Bq1PXqS1FNkFuju: i5ZE:iOEZP0P0EZNZN0NZq0qGNZu7S8NZ8Gq0qr  :,  ::SN88EGq0M@@                                                            
                       .vGNGkXXEEY,L52u8O8OMY:ZXkNqZSY1k52vJ.   :@: NME0PqPG0EqEP0qNqN0ZN0qOEEq0ZGGOZi,v7, :v@B@OOGO8OO@B                                                           
                        iM@MGqEqqu08OM@MMMGv, 5GqP0qE1u1q5YvL    7.:qZZ0NX00G0ZEEq0q0P0P0NOGZNGG88MM@Bui2i  .:PB@B@GZEMM@BL                                                         
                          7@B@MMB@B@B5B@B@OX87v@E0N0PZXXSuuuUi :; 5@OOZOM@B@MBMO8O8OOOG8GOOMM@B@B@B@@rv8v8: :i   :k@B@GOBBB@r                                                       
                           ,iLMBjri.  , ivEB@B@B@EEq0qZN5kuu21 . :@GZqGB@MEZMO@B@B@B@B@@@B@B@Zj7L7.    Y@.Svi77     .vYL0BY7kv                                                      
                                                L@N0S5qZPNqEX8;  @M8XPO@                                :v,OkY5u       2:O:   r                                                     
                                                 N@00qGNXXOOr;: uBOG0X@v                                 .L:SUkGG      FBr.  ,:                                                     
                                                  k@GZEZqkE@F. MBOGOO@B,                                  vBu:,.:r    Z@O1E@BX                                                      
                                                   B8NqkGPvL: 0@MO@B@G,                        ,...  .   :B@ZuFLr7:,.X@u:rBBu                                                       
                                                  .@00SXqr:, :B@B@Mi                         .7J7@B@E@q:7@MMM@BMZPE@B@P7ikB:                                                        
                                                 ,@ZEP0PS::;rB@BF                          :BSi:.0OOMN8B@MMB@BES@NUv7 ,iqB,                                                         
                                                r@OqP00P12X@B@i                          :iiZO.,vOqPFULkG@Bqii, rMG,  :kB,                                                          
                                             .kB@Z0qE0ZX0B@E.                           v@uX@MMM@8ZNESP0@j  .LY:7G@2.:LB,                                                           
                                            B@BOE0qGO@B@BL                 :             vr:7@B@B@B@B@B@M   Y@MMO@B@B@1                                                             
                                           .@FE0ENGM@Mi                    U.                 ...,,iku ,M:iv@B@B0;:,,                                                               
                                            @ONYrSG@7                                                   rr,   ,                                                                     
                                           .@ZP2P8Xu                                                                                                                                
                                           8BG08u..,                                                                                                                                
                                           BMqqEi .                                                                                                                                 
                                          1@GZPJr.:                                                                                                                                 
                                         B@OOOGL7.:                                                                                                                                 
                                        r8MXLr1EM: :               ,                                                                                                                
  ,                                     7qF7i :rMF.v,                                                                                                                               
     , ,   , , , , , , , ,             iNur7.,.L2@:uL           ,u,  :                                                                                                              
,         ,     , ,                    v07i:: :U1Zi7Fr          2O,  :Y.                                                                                    ,                       
, , ,,   ,  ,,  ,,                    .@7Liii.ivuui:.Gk         .:   ,.:,                                    ,     ,   ,  ,, ,,, , ,,.,....:, .,,   ,r.:,,,..:,, ,,,,,,  ,.,.,,i,ii,
  ,.,,.,. ,,.,.,,,  ,,,,, , ,,        7MSui.;L77uq0N@@@G07..iLi,.:ii::r7:::,..,,,.,::,.,..., ,....:i7i.ii.:::.:r::,,:, .....i.:.:......,,..,,,..,,:..  .,:.     . ..:.:.,,.r; :.;i:.
  ,.., :i,..               ,          :@BMv.F@FOONBG7     ..,, ,;i:i..,, .,,,:r7ii.r7:.i:7ivvii77r77:: i.,,, ::..:,.::,.,:.  ,, ,:.,,.:: .,,,..  :   , ::, ... ,v, ..i;:   . .,  ,  
          .   .,  .,  ,   ,,      ,   uJiuBLvi.iJE@L        : :.,          . :, ,   .,. ,   .                  ,         .              .   :,.,,.            .,   , ,i,..:.,:.,.  ,
            :,          ,.                                                         ,                                                 ,    ,    ,                  .. ,,. .   ,  ,  ,
  ..   ,        i                                                             ,                 ,   .,         ,                          ,                                         
 ,               ,                                          ,    .,.        ,,. ,,   ,,  i.   ,.,,:::..,        ., ,        , ,,     ,                          .                ,  
                                                    ,    ,  ,,.,   ,  ,   ,,         ,,..  ,:   .,     .. ,     ,     , .,, , ,           ,     :  ,, ..,   ,         ,           , 
                                 :   ,                  ,             ,            ,   .,                .  ,, ,   , .  ,                 ,,   :.   ,  ,                            
*/

// Results on Core 2 Q9550s @2.83GHz:
/*
D:\_KAZE\Nakamichi_Mustang>type GZIPit.bat
@echo off
if '%1'== '' goto usage
7za a -tgzip -mx9 %1.zip %1
goto quit
:usage
echo Usage: GZIPit filename
:quit

D:\_KAZE\Nakamichi_Mustang>dir

08/05/2015  03:30 AM           152,089 alice29.txt
08/05/2015  03:30 AM            72,058 alice29.txt.Nakamichi
08/05/2015  04:28 AM            51,707 alice29.txt.zip
07/26/2015  11:55 PM        10,192,446 dickens
08/05/2015  07:48 AM         3,776,411 dickens.Nakamichi
08/05/2015  04:32 AM         3,681,828 dickens.zip
07/26/2015  11:55 PM           260,569 Fahrenheit_451_-_Ray_Bradbury.txt
08/05/2015  03:31 AM           124,026 Fahrenheit_451_-_Ray_Bradbury.txt.Nakamichi
08/05/2015  04:28 AM            92,457 Fahrenheit_451_-_Ray_Bradbury.txt.zip
07/26/2015  11:55 PM         9,569,792 Fyodor_Dostoyevsky_12-books.tar
08/05/2015  05:59 AM         3,484,330 Fyodor_Dostoyevsky_12-books.tar.Nakamichi
08/05/2015  04:32 AM         3,386,527 Fyodor_Dostoyevsky_12-books.tar.zip
07/26/2015  11:55 PM           542,780 Hagakure_-_Tsunetoma_Yamamoto.txt
08/05/2015  03:34 AM           241,726 Hagakure_-_Tsunetoma_Yamamoto.txt.Nakamichi
08/05/2015  04:29 AM           198,660 Hagakure_-_Tsunetoma_Yamamoto.txt.zip
07/26/2015  11:55 PM            41,997 Ice_Cube_(1998)_-_War_and_Peace_(The_War_Disc)_-_Ghetto_Vet.mid
08/05/2015  03:32 AM             7,311 Ice_Cube_(1998)_-_War_and_Peace_(The_War_Disc)_-_Ghetto_Vet.mid.Nakamichi
08/05/2015  04:29 AM             1,949 Ice_Cube_(1998)_-_War_and_Peace_(The_War_Disc)_-_Ghetto_Vet.mid.zip
07/26/2015  11:55 PM            92,096 The_Little_Prince_-_Antoine_de_Saint-Exupery.epub.txt
08/05/2015  03:35 AM            43,905 The_Little_Prince_-_Antoine_de_Saint-Exupery.epub.txt.Nakamichi
08/05/2015  04:29 AM            30,329 The_Little_Prince_-_Antoine_de_Saint-Exupery.epub.txt.zip
08/05/2015  03:53 AM         3,265,536 University_of_Canterbury_The_Calgary_Corpus.tar
08/05/2015  02:56 AM         1,316,869 University_of_Canterbury_The_Calgary_Corpus.tar.Nakamichi
08/05/2015  04:29 AM         1,017,658 University_of_Canterbury_The_Calgary_Corpus.tar.zip
08/05/2015  03:53 AM         1,696,256 zpaq705.tar
08/05/2015  03:52 AM           836,036 zpaq705.tar.Nakamichi
08/05/2015  04:29 AM           603,143 zpaq705.tar.zip

D:\_KAZE\Nakamichi_Mustang>Nakamichi_Mustang_branchfull.exe dickens.Nakamichi /report
Nakamichi 'Mustang', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 3776411 bytes ...
RAM-to-RAM performance: 448 MB/s.
Memory pool starting address: 00000000008B0080 ... 64 byte aligned, OK
Copying a 512MB block 1024 times i.e. 512GB READ + 512GB WRITTEN ...
^C

D:\_KAZE\Nakamichi_Mustang>Nakamichi_Mustang_branchfull.exe University_of_Canterbury_The_Calgary_Corpus.tar.Nakamichi /report
Nakamichi 'Mustang', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 1316869 bytes ...
RAM-to-RAM performance: 576 MB/s.
Memory pool starting address: 0000000000690080 ... 64 byte aligned, OK
Copying a 512MB block 1024 times i.e. 512GB READ + 512GB WRITTEN ...
^C

D:\_KAZE\Nakamichi_Mustang>\lz4 -9 -b -Sx -T1 dickens
Nb of threads = 1 ; Compression Level = 9
dickens         :  10192446 ->   4442965 ( 43.59%),   11.8 MB/s ,  896.2 MB/s

D:\_KAZE\Nakamichi_Mustang>\lz4 -9 -b -Sx -T1 University_of_Canterbury_The_Calgary_Corpus.tar
Nb of threads = 1 ; Compression Level = 9
University_of_C :   3265536 ->   1241281 ( 38.01%),   16.1 MB/s , 1059.8 MB/s

D:\_KAZE\Nakamichi_Mustang>
*/

/*
D:\_KAZE\Nakamichi_Mustang>Nakamichi_Mustang_branchfull.exe Fyodor_Dostoyevsky_12-books.tar
Nakamichi 'Mustang', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Compressing 9569792 bytes ...
-; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 24
NumberOfFlushLiteralsHeuristic (bigger-the-better): 69193
Legend: WindowSizes: 1/2/3/4=Tiny/Short/Medium/Long
NumberOf(Tiny)Matches[Tiny]Window (2)[1]: 0
NumberOf(Short)Matches[Tiny]Window (6)[1]: 2154
NumberOf(Medium)Matches[Tiny]Window (10)[1]: 489
NumberOf(Long)Matches[Tiny]Window (14)[1]: 766
NumberOf(Tiny)Matches[Short]Window (3)[2]: 234763
NumberOf(Short)Matches[Short]Window (7)[2]: 85702
NumberOf(Medium)Matches[Short]Window (11)[2]: 26473
NumberOf(Long)Matches[Short]Window (15)[2]: 16428
NumberOf(Tiny)Matches[Medium]Window (4)[3]: 108216
NumberOf(Short)Matches[Medium]Window (8)[3]: 299852
NumberOf(Medium)Matches[Medium]Window (12)[3]: 169245
NumberOf(Long)Matches[Medium]Window (16)[3]: 59524
NumberOf(Tiny)Matches[Long]Window (5)[4]: 6072
NumberOf(Short)Matches[Long]Window (9)[4]: 69294
NumberOf(Medium)Matches[Long]Window (13)[4]: 86457
RAM-to-RAM performance: 1847 bytes/s.
Compressed to 3484330 bytes.

D:\_KAZE\Nakamichi_Mustang>7za.exe l Fyodor_Dostoyevsky_12-books.tar

7-Zip (A) 9.20  Copyright (c) 1999-2010 Igor Pavlov  2010-11-18

Listing archive: Fyodor_Dostoyevsky_12-books.tar

--
Path = Fyodor_Dostoyevsky_12-books.tar
Type = tar
Physical Size = 9569792
Headers Size = 9216

   Date      Time    Attr         Size   Compressed  Name
------------------- ----- ------------ ------------  ------------------------
2015-05-30 01:09:07 .....       613108       613376  Fyodor_Dostoyevsky_12-books\38241-0_Uncle's_Dream_and_The_Permanent_Husband_by_Fyodor_Dostoyevsky.txt
2015-05-30 01:01:30 .....        75416        75776  Fyodor_Dostoyevsky_12-books\8578_The_Grand_Inquisitor_by_Fyodor_Dostoyevsky.txt
2015-05-30 01:03:36 .....       357764       357888  Fyodor_Dostoyevsky_12-books\pg2197_The_Gambler_by_Fyodor_Dostoyevsky.txt
2015-05-30 01:07:38 .....       316364       316416  Fyodor_Dostoyevsky_12-books\pg2302_Poor_Folk_by_Fyodor_Dostoyevsky.txt
2015-05-30 00:49:08 .....      1177123      1177600  Fyodor_Dostoyevsky_12-books\pg2554_Crime_and_Punishment_by_Fyodor_Dostoyevsky.txt
2015-05-30 00:51:15 .....      1396972      1397248  Fyodor_Dostoyevsky_12-books\pg2638_The_Idiot_by_Fyodor_Dostoyevsky.txt
2015-05-30 00:44:48 .....      1996603      1996800  Fyodor_Dostoyevsky_12-books\pg28054_The_Brothers_Karamazov_by_Fyodor_Dostoyevsky.txt
2015-05-30 00:54:18 .....       682370       682496  Fyodor_Dostoyevsky_12-books\pg36034_White_Nights_and_Other_Stories_by_Fyodor_Dostoyevsky.txt
2015-05-30 01:05:33 .....       717666       717824  Fyodor_Dostoyevsky_12-books\pg37536_The_House_of_the_Dead_or_Prison_Life_in_Siberia_by_Fyodor_Dostoyevsky.txt
2015-05-30 00:57:42 .....       475679       476160  Fyodor_Dostoyevsky_12-books\pg40745_Short_Stories_by_Fyodor_Dostoyevsky.txt
2015-05-30 00:50:14 .....       265585       265728  Fyodor_Dostoyevsky_12-books\pg600_Notes_from_the_Underground_by_Fyodor_Dostoyevsky.txt
2015-05-30 00:52:56 .....      1482966      1483264  Fyodor_Dostoyevsky_12-books\pg8117_The_Possessed_(The_Devils)_by_Fyodor_Dostoyevsky.txt
------------------- ----- ------------ ------------  ------------------------
                               9557616      9560576  12 files, 0 folders

D:\_KAZE\Nakamichi_Mustang>

[Performers:]

- LZ4 for Windows 32-bits v1.4, by Yann Collet (Sep 17 2013).
- zstd command line interface 64-bits v0.0.1, by Yann Collet (Jan 25 2015).
- 7-Zip (A) 9.20, Copyright (c) 1999-2010 Igor Pavlov, 2010-11-18.
- bsc, Block Sorting Compressor, Version 3.1.0. Copyright (c) 2009-2012 Ilya Grebnov, 8 July 2012.
- lzturbo 1.2 Copyright (c) 2007-2014 Hamid Buzidi, Aug 11 2014.
- zpaq v7.05 journaling archiver, compiled Apr 17 2015.
- Nakamichi 'Oniyanma-Monsterdragonfly-Lexx' (branchless 64bit compile), written by Kaze, based on Nobuo Ito's LZSS source, 2015-Mar-27.
- Nakamichi 'Akuuma-Mustang' (branchfull 64bit compile), written by Kaze, based on Nobuo Ito's LZSS source, 2015-Aug-04.

[Options used (as in):]

>LZ4.exe -9 enwik8
>zstd.exe enwik8
>7za a -tgzip -mx9 enwik8.zip enwik8
>bsc e enwik8 enwik8.ST6Block256.bsc -b256 -m6 -cp -Tt
>lzturbo.exe -39 -b256 -p0 enwik8 enwik8.256MB.lzturbo12-39.lzt
>zpaq64.exe add enwik8.method28.zpaq enwik8 -method 28 -threads 1
>zpaq64.exe add enwik8.method58.zpaq enwik8 -method 58 -threads 1
>Nakamichi_Oniyanma_Monsterdragonfly_Lexx_GP_64bit.exe enwik8
>Nakamichi_Mustang_branchfull.exe enwik8

['Roster':]

06/02/2015  11:27 PM         9,569,792 Fyodor_Dostoyevsky_12-books.tar
06/03/2015  02:59 AM         4,080,915 Fyodor_Dostoyevsky_12-books.tar.lz4
06/03/2015  03:00 AM         3,820,603 Fyodor_Dostoyevsky_12-books.tar.zst
06/01/2015  12:17 AM         3,508,317 Fyodor_Dostoyevsky_12-books.tar.Lexx.Nakamichi    ! Requires native 256bit system to perform as it should !
08/05/2015  05:59 AM         3,484,330 Fyodor_Dostoyevsky_12-books.tar.Mustang.Nakamichi ! Much faster than 'Lexx', it uses 128bit registers !
06/03/2015  03:05 AM         3,386,527 Fyodor_Dostoyevsky_12-books.tar.zip
06/03/2015  03:29 AM         3,062,047 Fyodor_Dostoyevsky_12-books.tar.method28.zpaq
06/03/2015  02:38 AM         2,560,124 Fyodor_Dostoyevsky_12-books.tar.256MB.lzturbo12-39.lzt
06/03/2015  03:26 AM         2,122,550 Fyodor_Dostoyevsky_12-books.tar.ST6Block256.bsc
06/03/2015  03:55 AM         1,882,135 Fyodor_Dostoyevsky_12-books.tar.method58.zpaq
*/

// The formal name is 'Loonette Hoshimi Dame'.
// The informal name is 'The Rabbitlet Girl Stargazing'.

// Sir is an honorific address used in a number of situations in many anglophone cultures.
// Equivalent terms of address to females are "ma'am" or "madam" in most cases, or in the case of a very young woman, girl, or unmarried woman who prefers to be addressed as such, "miss".
// The equivalent term for a knighted woman or baronetess is Dame, or "Lady" for the wife of a knight or baronet.

// Core 2 Q9550s:
/*
D:\_KAZE\Nakamichi_Loonette_Hoshimi_vs_enwik8>Nakamichi_Loonette-Hoshimi_branchless.exe enwik8.Nakamichi /bench
Nakamichi 'Loonette-Hoshimi', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 34968896 bytes ...
RAM-to-RAM performance: 192 MB/s.
Memory pool starting address: 0000000002620080 ... 64 byte aligned, OK
Copying a 512MB block 1024 times i.e. 512GB READ + 512GB WRITTEN ...
memcpy(): (512MB block); 524288MB copied in 193784 clocks or 2.706MB per clock
RAM-to-RAM performance vs memcpy() ratio (bigger-the-better): 7%

D:\_KAZE\Nakamichi_Loonette_Hoshimi_vs_enwik8>\sha1sum.exe enwik8
57b8363b814821dc9d47aa4d41f58733519076b2  enwik8

D:\_KAZE\Nakamichi_Loonette_Hoshimi_vs_enwik8>Nakamichi_Loonette_Hoshimi_XMM_PREFETCH_4096.exe enwik8.Nakamichi /bench
Nakamichi 'Loonette-Hoshimi', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 34968896 bytes ...
RAM-to-RAM performance: 256 MB/s.
Memory pool starting address: 0000000002710080 ... 64 byte aligned, OK
Copying a 512MB block 1024 times i.e. 512GB READ + 512GB WRITTEN ...
memcpy(): (512MB block); 524288MB copied in 193453 clocks or 2.710MB per clock
RAM-to-RAM performance vs memcpy() ratio (bigger-the-better): 9%

D:\_KAZE\Nakamichi_Loonette_Hoshimi_vs_enwik8>\sha1sum.exe enwik8
57b8363b814821dc9d47aa4d41f58733519076b2  enwik8

D:\_KAZE\Nakamichi_Loonette_Hoshimi_vs_enwik8>
*/

/*
Shunkan

twinkle
v. intr.
1. To shine with slight, intermittent gleams, as distant lights or stars; flicker; glimmer. See Synonyms at flash.
2. To be bright or sparkling, as with merriment or delight: eyes that twinkled with joy. 
3. To blink or wink the eyes. See Synonyms at blink.
4. To move about or to and fro rapidly and gracefully; flit.
v. tr.
To emit (light) in slight, intermittent gleams.
n. 
1. A slight, intermittent gleam of light; a sparkling flash; a glimmer.
2. A sparkle of merriment or delight in the eye.
3. A brief interval; a twinkling.
4. A rapid to-and-fro movement.
[Middle English twinklen, from Old English twinclian, frequentative of twincan, to blink.]
*/

// silk used in artwork : kenpon
// Kinu - silk, Kinuno - silken
// tiger : tora

// Nakamichi_Rakka.c, using 16B/4KB/1MB/2MB or (8-4)bit/(16-4)bit/(24-4)bit/(32-4)bit windows with 1/2/3/4 bytes long offsets.
// Nakamichi_Kokuen.c, using 16B/4KB/1MB/256MB or (8-4)bit/(16-4)bit/(24-4)bit/(32-4)bit windows with 1/2/3/4 bytes long offsets.
// Nakamichi_Yoko.c, using 16B/4KB/1MB or (8-4)bit/(16-4)bit/(24-4)bit windows with 1/2/3 bytes long offsets.
// Nakamichi_Kinutora.c, using 4KB/1MB or (16-4)bit/(24-4)bit windows with 2/3 bytes long offsets.
// Nakamichi_Butsuhira.c, using 8KB/2MB or (16-3)bit/(24-3)bit windows with 2/3 bytes long offsets.
// Nakamichi_Kinroba.c, using 4KB/1MB/256MB or 12bit/20bit/28bit windows with 2/3/4 bytes long offsets.
// Nakamichi_Keigan.c, a branchless 'Kaibutsu' it is using 2MB/512MB or 21bit/29bit windows with 3/4 bytes long offsets.
// Nakamichi_Washi.c, a branchless 'Kaibutsu' it is using 4MB window.

// Nakamichi_Kaiju.c, a branchless 'Kaibutsu' it is.
// ML=9
// 68,352,060 enwik8.Kaiju.Nakamichi
// ML=8
// 63,748,036 enwik8.Kaiju.Nakamichi
// ML=7
// 59,771,603 enwik8.Kaiju.Nakamichi
// ML=6
// 57,090,382 enwik8.Kaiju.Nakamichi
// ML=5
// 56,188,976
// ML=4
// 58,954,436 enwik8.Kaiju.Nakamichi
// Nakamichi_Kaibutsu.c, three small tweaks in Kaidanji, a good idea to remove shiftings altogether by m^2 was used.
// Nakamichi_Kaidanji.c, is the very same '1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX'.

// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX, written by Kaze, babealicious suggestion by m^2 enforced.
// Fixed! TO-DO: Known bug: the decompressed file sometimes has few additional bytes at the end.
// Change #1: Now instead of looking first in the leftmost end of the window a "preemptive" search is done 16*8*128 bytes before the rightmost end of the window, there is the hottest (cachewise&matchwise) zone, as a side effect the compression speed is much higher. Maybe in the future I will try hashing as well.
// Change #2: The full 16bits are used for offsets, 64KB window, that is.

// Compile line:
//icl /O3 Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.c -D_N_GP /FAcs
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.cod Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_GP.cod
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.exe Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_GP.exe
//icl /O3 /QxSSE2 Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.c -D_N_XMM /FAcs
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.cod Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_XMM.cod
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.exe Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_XMM.exe
//icl /O3 /QxAVX Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.c -D_N_YMM /FAcs
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.cod Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_YMM.cod
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.exe Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_YMM.exe

// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX, written by Kaze, babealicious suggestion by m^2 enforced.
// Change #1: Nasty bug in Swampshine was fixed.
// Change #2: Sanity check in compression section was added thus avoiding 'index-Min_Match_Length' going below 0.

// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy, written by Kaze, babealicious suggestion by m^2 enforced.
// Change #1: 'memcpy' replaced by GP/XMM/YMM TRIADs.
// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD, written by Kaze.
// Change #1: Decompression fetches WORD instead of BYTE+BYTE.
// Change #2: Decompression stores three times 64bit instead of memcpy() for all transfers <=24 bytes.
// Change #3: Fifteenth bit is used and then unused, 16KB -> 32KB -> 16KB.
// 32KB window disappoints speedwise, also sizewise:
/*
D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik8
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Compressing 100000000 bytes ...
-; Each rotation means 128KB are encoded; Done 100%
RAM-to-RAM performance: 130 KB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik8.Nakamichi
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Decompressing 65693566 bytes ...
RAM-to-RAM performance: 358 MB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik9
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Compressing 1000000000 bytes ...
/; Each rotation means 128KB are encoded; Done 100%
RAM-to-RAM performance: 150 KB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik9.Nakamichi
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Decompressing 609319736 bytes ...
RAM-to-RAM performance: 379 MB/s.
*/
// 1-RSSBO_1GB vs 1-RSSBO_1GB_15bit_Wordfetcher (16KB/32KB respectively):
// 069,443,065 vs 065,693,566
// 641,441,055 vs 609,319,736

// Nakamichi, revision 1-RSSBO_1GB, written by Kaze.
// Based on Nobuo Ito's source, thanks Ito.
// The main goal of Nakamichi is to allow supersimple and superfast decoding for English x-grams (mainly) in pure C, or not, heh-heh.
// Natively Nakamichi is targeted as 64bit tool with 16 threads, helping Kazahana to traverse faster when I/O is not superior.
// In short, Nakamichi is intended as x-gram decompressor.

// Eightfold Path ~ the Buddhist path to nirvana, comprising eight aspects in which an aspirant must become practised; 
// eightfold way ~ (Physics), the grouping of hadrons into supermultiplets by means of SU(3)); (b) adverb to eight times the number or quantity: OE.

// Note1: Fifteenth bit is not used, making the window wider by 1bit i.e. 32KB is not tempting, rather I think to use it as a flag: 8bytes/16bytes.
// Note2: English x-grams are as English texts but more redundant, in other words they are phraselists in most cases, sometimes wordlists.
// Note3: On OSHO.TXT, being a typical English text, Nakamichi's compression ratio is among the worsts:
//        206,908,949 OSHO.TXT
//        125,022,859 OSHO.TXT.Nakamichi
//        It struggles with English texts but decomprression speed is quite sweet (Core 2 T7500 2200MHz, 32bit code):
//        Nakamichi, revision 1-, written by Kaze.
//        Decompressing 125022859 bytes ...
//        RAM-to-RAM performance: 477681 KB/s.      
// Note4: Also I wanted to see how my 'Railgun_Swampshine_BailOut', being a HEAVYGUN i.e. with big overhead and latency, hits in a real-world application.

// Quick notes on PAGODAs (the padded x-gram lists):
// Every single word in English has its own PAGODA, in example below 'on' PAGODA is given (Kazahana_on.PAGODA-order-5.txt):
// PAGODA order 5 (i.e. with 5 tiers) has 5*(5+1)/2=15 subtiers, they are concatenated and space-padded in order to form the pillar 'on':
/*
D:\_KAZE\Nakamichi_r1-RSSBO>dir \_GW\ka*

04/12/2014  05:07 AM                14 Kazahana_on.1-1.txt
04/12/2014  05:07 AM         1,635,389 Kazahana_on.2-1.txt
04/12/2014  05:07 AM         1,906,734 Kazahana_on.2-2.txt
04/12/2014  05:07 AM        10,891,415 Kazahana_on.3-1.txt
04/12/2014  05:07 AM        15,797,703 Kazahana_on.3-2.txt
04/12/2014  05:07 AM        20,419,280 Kazahana_on.3-3.txt
04/12/2014  05:07 AM        22,141,823 Kazahana_on.4-1.txt
04/12/2014  05:07 AM        36,002,113 Kazahana_on.4-2.txt
04/12/2014  05:07 AM        33,236,772 Kazahana_on.4-3.txt
04/12/2014  05:07 AM        33,902,425 Kazahana_on.4-4.txt
04/12/2014  05:07 AM        24,795,989 Kazahana_on.5-1.txt
04/12/2014  05:07 AM        30,766,220 Kazahana_on.5-2.txt
04/12/2014  05:07 AM        38,982,816 Kazahana_on.5-3.txt
04/12/2014  05:07 AM        38,089,575 Kazahana_on.5-4.txt
04/12/2014  05:07 AM        34,309,057 Kazahana_on.5-5.txt
04/12/2014  05:07 AM       846,351,894 Kazahana_on.PAGODA-order-5.txt

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.1-1.txt
9,999,999       on

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.2-1.txt
9,999,999       on_the
1,148,054       on_his
0,559,694       on_her
0,487,856       on_this
0,399,485       on_your
0,381,570       on_my
0,367,282       on_their
...

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.2-2.txt
0,545,191       based_on
0,397,408       and_on
0,334,266       go_on
0,329,561       went_on
0,263,035       was_on
0,246,332       it_on
0,229,041       down_on
0,202,151       going_on
...

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.5-5.txt
0,083,564       foundation_osho_s_books_on
0,012,404       medium_it_may_be_on
0,012,354       if_you_received_it_on
0,012,152       medium_they_may_be_on
0,012,144       agree_to_also_provide_on
0,012,139       a_united_states_copyright_on
0,008,067       we_are_constantly_working_on
0,008,067       questions_we_have_received_on
0,006,847       file_was_first_posted_on
0,006,441       of_we_are_already_on
0,006,279       you_received_this_ebook_on
0,005,865       you_received_this_etext_on
0,005,833       to_keep_an_eye_on
...

D:\_KAZE\Nakamichi_r1-RSSBO>dir

04/12/2014  05:07 AM       846,351,894 Kazahana_on.PAGODA-order-5.txt

D:\_KAZE\Nakamichi_r1-RSSBO>Nakamichi.exe Kazahana_on.PAGODA-order-5.txt
Nakamichi, revision 1-RSSBO, written by Kaze.
Compressing 846351894 bytes ...
/; Each rotation means 128KB are encoded; Done 100%
RAM-to-RAM performance: 512 KB/s.

D:\_KAZE\Nakamichi_r1-RSSBO>dir

04/12/2014  05:07 AM       846,351,894 Kazahana_on.PAGODA-order-5.txt
04/15/2014  06:30 PM       293,049,398 Kazahana_on.PAGODA-order-5.txt.Nakamichi

D:\_KAZE\Nakamichi_r1-RSSBO>Nakamichi.exe Kazahana_on.PAGODA-order-5.txt.Nakamichi
Nakamichi, revision 1-RSSBO, written by Kaze.
Decompressing 293049398 bytes ...
RAM-to-RAM performance: 607 MB/s.

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 4096
YAPPY: [b 4K] bytes 846351894 -> 191149889  22.6%  comp  33.8 MB/s  uncomp 875.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 8192
YAPPY: [b 8K] bytes 846351894 -> 184153244  21.8%  comp  35.0 MB/s  uncomp 898.3 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 16384
YAPPY: [b 16K] bytes 846351894 -> 180650931  21.3%  comp  28.8 MB/s  uncomp 906.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 32768
YAPPY: [b 32K] bytes 846351894 -> 178902966  21.1%  comp  35.0 MB/s  uncomp 906.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 65536
YAPPY: [b 64K] bytes 846351894 -> 178027899  21.0%  comp  34.5 MB/s  uncomp 914.6 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 131072
YAPPY: [b 128K] bytes 846351894 -> 177591807  21.0%  comp  34.9 MB/s  uncomp 906.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>
*/
#define _N_Branchfull
// During compilation use one of these, the granularity of the padded 'memcpy', 4x2x8/2x2x16/1x2x32/1x1x64 respectively as GP/XMM/YMM/ZMM, the maximum literal length reduced from 127 to 63:
//#define _N_GP
#define _N_XMM
//#define _N_YMM
//#define _N_ZMM

//#define _N_prefetch_64
//#define _N_prefetch_128
//#define _N_prefetch_4096

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // uint64_t needed
#include <time.h>
#include <string.h>

#ifdef _N_XMM
#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics
#endif
#ifdef _N_YMM
#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics
#include <immintrin.h> // AVX intrinsics
#endif
#ifdef _N_ZMM
#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics
#include <immintrin.h> // AVX intrinsics
#include <zmmintrin.h> // AVX2 intrinsics, definitions and declarations for use with 512-bit compiler intrinsics.
#endif

#ifdef _N_XMM
void SlowCopy128bit (const char *SOURCE, char *TARGET) { _mm_storeu_si128((__m128i *)(TARGET), _mm_loadu_si128((const __m128i *)(SOURCE))); }
#endif
#ifdef _N_YMM
void SlowCopy128bit (const char *SOURCE, char *TARGET) { _mm_storeu_si128((__m128i *)(TARGET), _mm_loadu_si128((const __m128i *)(SOURCE))); }
#endif
#ifdef _N_ZMM
void SlowCopy128bit (const char *SOURCE, char *TARGET) { _mm_storeu_si128((__m128i *)(TARGET), _mm_loadu_si128((const __m128i *)(SOURCE))); }
#endif
/*
 * Move Unaligned Packed Integer Values
 * **** VMOVDQU ymm1, m256
 * **** VMOVDQU m256, ymm1
 * Moves 256 bits of packed integer values from the source operand to the
 * destination
 */
//extern __m256i __ICL_INTRINCC _mm256_loadu_si256(__m256i const *);
//extern void    __ICL_INTRINCC _mm256_storeu_si256(__m256i *, __m256i);
#ifdef _N_YMM
void SlowCopy256bit (const char *SOURCE, char *TARGET) { _mm256_storeu_si256((__m256i *)(TARGET), _mm256_loadu_si256((const __m256i *)(SOURCE))); }
#endif
//extern __m512i __ICL_INTRINCC _mm512_loadu_si512(void const*);
//extern void    __ICL_INTRINCC _mm512_storeu_si512(void*, __m512i);
#ifdef _N_ZMM
void SlowCopy512bit (const char *SOURCE, char *TARGET) { _mm512_storeu_si512((__m512i *)(TARGET), _mm512_loadu_si512((const __m512i *)(SOURCE))); }
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

// Comment it to see how slower 'BruteForce' is, for Wikipedia 100MB the ratio is 41KB/s versus 197KB/s.
#define ReplaceBruteForceWithRailgunSwampshineBailOut

void SearchIntoSlidingWindow(unsigned int* ShortMediumLongOFFSET, unsigned int* retIndex, unsigned int* retMatch, char* refStart,char* refEnd,char* encStart,char* encEnd);
unsigned int SlidingWindowVsLookAheadBuffer(char* refStart, char* refEnd, char* encStart, char* encEnd);
unsigned int NakaCompress(char* ret, char* src, unsigned int srcSize);
uint64_t NakaDecompress(char* ret, char* src, uint64_t srcSize);
char * Railgun_Swampshine_BailOut(char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern);
char * Railgun_Doublet (char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern);

/*
void memcpy_AVX_4K_prefetched (void *dst, const void *src, size_t nbytes) {
// F3 0F 6F /r RM V/V SSE2 Move unaligned packed integer values from xmm2/m128 to xmm1.
// MOVDQU xmm1, xmm2/m128 
// F3 0F 7F /r MR V/V SSE2 Move unaligned packed integer values from xmm1 to xmm2/m128.
// MOVDQU xmm2/m128, xmm1 
// VEX.128.F3.0F.WIG 6F /r RM V/V AVX Move unaligned packed integer values from xmm2/m128 to xmm1.
// VMOVDQU xmm1, xmm2/m128 
// VEX.128.F3.0F.WIG 7F /r MR V/V AVX Move unaligned packed integer values from xmm1 to xmm2/m128.
// VMOVDQU xmm2/m128, xmm1 
// VEX.256.F3.0F.WIG 6F /r RM V/V AVX Move unaligned packed integer values from ymm2/m256 to ymm1.
// VMOVDQU ymm1, ymm2/m256 
// VEX.256.F3.0F.WIG 7F /r MR V/V AVX Move unaligned packed integer values from ymm1 to ymm2/m256.
// VMOVDQU ymm2/m256, ymm1 
if ( (nbytes&0x3f) == 0 ) { // 64bytes per cycle
	__asm{
		mov		rsi, src
		mov		rdi, dst
		mov		rcx, nbytes
		shr		rcx, 6
	main_loop:
		test		rcx, rcx ; 'nbytes' may be 0
		jz		main_loop_end
		prefetcht0	[rsi+64*64]
		vmovdqu		xmm0, [rsi]
		vmovdqu		xmm1, [rsi+16]
		vmovdqu		xmm2, [rsi+32]
		vmovdqu		xmm3, [rsi+48]
		vmovdqu		[rdi], xmm0
		vmovdqu		[rdi+16], xmm1
		vmovdqu		[rdi+32], xmm2
		vmovdqu		[rdi+48], xmm3
		add		rsi, 64
		add		rdi, 64
		dec		rcx
		jmp		main_loop
	main_loop_end:
		sfence
	}
} else memcpy(dst, src, nbytes);
}

void memcpy_SSE2_4K_prefetched (void *dst, const void *src, size_t nbytes) {
// F3 0F 6F /r RM V/V SSE2 Move unaligned packed integer values from xmm2/m128 to xmm1.
// MOVDQU xmm1, xmm2/m128 
// F3 0F 7F /r MR V/V SSE2 Move unaligned packed integer values from xmm1 to xmm2/m128.
// MOVDQU xmm2/m128, xmm1 
// VEX.128.F3.0F.WIG 6F /r RM V/V AVX Move unaligned packed integer values from xmm2/m128 to xmm1.
// VMOVDQU xmm1, xmm2/m128 
// VEX.128.F3.0F.WIG 7F /r MR V/V AVX Move unaligned packed integer values from xmm1 to xmm2/m128.
// VMOVDQU xmm2/m128, xmm1 
// VEX.256.F3.0F.WIG 6F /r RM V/V AVX Move unaligned packed integer values from ymm2/m256 to ymm1.
// VMOVDQU ymm1, ymm2/m256 
// VEX.256.F3.0F.WIG 7F /r MR V/V AVX Move unaligned packed integer values from ymm1 to ymm2/m256.
// VMOVDQU ymm2/m256, ymm1 
if ( (nbytes&0x3f) == 0 ) { // 64bytes per cycle
	__asm{
		mov		rsi, src
	        mov		rdi, dst
	        mov		rcx, nbytes
	        shr		rcx, 6
	main_loop:
		test		rcx, rcx ; 'nbytes' may be 0
	        jz		main_loop_end
	        prefetcht0	[rsi+64*64]
		movdqu 		xmm0, [rsi]
	        movdqu 		xmm1, [rsi+16]
	        movdqu 		xmm2, [rsi+32]
	        movdqu 		xmm3, [rsi+48]
		movdqu 		[rdi], xmm0
	        movdqu 		[rdi+16], xmm1
	        movdqu 		[rdi+32], xmm2
	        movdqu 		[rdi+48], xmm3
	        add		rsi, 64
	        add		rdi, 64
		dec		rcx
	        jmp		main_loop
	main_loop_end:
	        sfence
	}
} else memcpy(dst, src, nbytes);
}
*/

// Min_Match_Length=THRESHOLD=4 means 4 and bigger are to be encoded:
#define Min_Match_BAILOUT_Length (8)
#define Min_Match_Length (32)
#define Min_Match_Length_SHORT (5)
#define OffsetBITS (32-3)
#define LengthBITS (1)

//12bit
//#define REF_SIZE (4095+Min_Match_Length)
//#define REF_SIZE ( ((1<<OffsetBITS)-1) + Min_Match_Length )
#define REF_SIZE ( ((1<<OffsetBITS)-1) )
//3bit
//#define ENC_SIZE (7+Min_Match_Length)
#define ENC_SIZE ( ((1<<LengthBITS)-1) + Min_Match_Length )
#if 0
int main( int argc, char *argv[] ) {
	FILE *fp;
	int SourceSize;
	uint64_t TargetSize;
	char* SourceBlock=NULL;
	char* TargetBlock=NULL;
	char* Nakamichi = ".Nakamichi\0";
	char NewFileName[256];
	clock_t clocks1, clocks2;

char *pointerALIGN;
int i, j;
clock_t clocks3, clocks4;
double duration;
double durationGENERIC;
int BandwidthFlag=0;

unsigned long long k;
int Trials;

	printf("Nakamichi 'Kintaro', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.\n");
	printf("Note: This compile can handle files up to 1711MB.\n");
	if (argc==1) {
		printf("Usage: Nakamichi filename\n"); exit(13);
	}
	if (argc==3) BandwidthFlag=1;
	if (BandwidthFlag) Trials=64; else Trials=1; 
	if ((fp = fopen(argv[1], "rb")) == NULL) {
		printf("Nakamichi: Can't open '%s' file.\n", argv[1]); exit(13);
	}
	fseek(fp, 0, SEEK_END);
	SourceSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	// If filename ends in '.Nakamichi' then mode is decompression otherwise compression.
	if (strcmp(argv[1]+(strlen(argv[1])-strlen(Nakamichi)), Nakamichi) == 0) {
	SourceBlock = (char*)malloc(SourceSize+512);
	//TargetBlock = (char*)malloc(5*SourceSize+512);
	TargetBlock = (char*)malloc(1711*1024*1024+512);
	fread(SourceBlock, 1, SourceSize, fp);
	fclose(fp);
		printf("Decompressing %d bytes ...\n", SourceSize );

		clocks1 = clock();
		while (clocks1 == clock());
		clocks1 = clock();
	for (i = 1; i <= Trials; i++) {
		TargetSize = Decompress(TargetBlock, SourceBlock, SourceSize);
	}
		clocks2 = clock();
		//k = (((float)1000*TargetSize/(clocks2 - clocks1 + 1))); k=k>>20; k=k*Trials;
		k = (((float)CLOCKS_PER_SEC*TargetSize/(clocks2 - clocks1 + 1))); k=k>>20; k=k*Trials;
		printf("RAM-to-RAM performance: %d MB/s.\n", k);
		printf("Compression Ratio (bigger-the-better): %.2f:1\n", (float)(TargetSize) / (float)(SourceSize) );
		strcpy(NewFileName, argv[1]);
		*( NewFileName + strlen(argv[1])-strlen(Nakamichi) ) = '\0';
	} else {
	SourceBlock = (char*)malloc(SourceSize+512);
	TargetBlock = (char*)malloc(SourceSize+512+32*1024*1024); //+32*1024*1024, some files may be expanded instead of compressed.
	fread(SourceBlock, 1, SourceSize, fp);
	fclose(fp);
		printf("Compressing %d bytes ...\n", SourceSize );
		clocks1 = clock();
		while (clocks1 == clock());
		clocks1 = clock();
		TargetSize = Compress(TargetBlock, SourceBlock, SourceSize);
		clocks2 = clock();
		//k = (((float)1000*SourceSize/(clocks2 - clocks1 + 1))); //k=k>>10;
		k = (((float)CLOCKS_PER_SEC*SourceSize/(clocks2 - clocks1 + 1))); //k=k>>10;
		printf("RAM-to-RAM performance: %d bytes/s.\n", k);
		strcpy(NewFileName, argv[1]);
		strcat(NewFileName, Nakamichi);
		printf("Compressed to %d bytes.\n", TargetSize );
	}
	if ((fp = fopen(NewFileName, "wb")) == NULL) {
		printf("Nakamichi: Can't write '%s' file.\n", NewFileName); exit(13);
	}
	fwrite(TargetBlock, 1, TargetSize, fp);
	fclose(fp);

	//if (BandwidthFlag) {
	if (BandwidthFlag==2) { // 2 means disabled
// Benchmark memcpy() [
pointerALIGN = TargetBlock + 64 - (((size_t)TargetBlock) % 64);
//offset=64-int((long)data&63);
printf("Memory pool starting address: %p ... ", pointerALIGN);
if (((uintptr_t)(const void *)pointerALIGN & (64 - 1)) == 0) printf( "64 byte aligned, OK\n"); else printf( "NOT 64 byte aligned, FAILURE\n");
clocks3 = clock();
while (clocks3 == clock());
clocks3 = clock();
printf("Copying a %dMB block 1024 times i.e. %dGB READ + %dGB WRITTEN ...\n", 512, 512, 512);
	for (i = 0; i < 1024; i++) {
	memcpy(pointerALIGN+512*1024*1024, pointerALIGN, 512*1024*1024);
	}
clocks4 = clock();
duration = (double) (clocks4 - clocks3 + 1);
durationGENERIC = duration;
printf("memcpy(): (%dMB block); %dMB copied in %d clocks or %.3fMB per clock\n", 512, 1024*( 512 ), (int) duration, (float)1024*( 512 )/ ((int) duration));

/*
#ifndef _N_GP
clocks3 = clock();
while (clocks3 == clock());
clocks3 = clock();
printf("Copying a %dMB block 1024 times i.e. %dGB READ + %dGB WRITTEN ...\n", 512, 512, 512);
	for (i = 0; i < 1024; i++) {
	memcpy_SSE2_4K_prefetched(pointerALIGN+512*1024*1024, pointerALIGN, 512*1024*1024);
	}
clocks4 = clock();
duration = (double) (clocks4 - clocks3 + 1);
printf("memcpy_SSE2_4K_prefetched(): (%dMB block); %dMB copied in %d clocks or %.3fMB per clock\n", 512, 1024*( 512 ), (int) duration, (float)1024*( 512 )/ ((int) duration));
#endif

#ifdef _N_YMM
clocks3 = clock();
while (clocks3 == clock());
clocks3 = clock();
printf("Copying a %dMB block 1024 times i.e. %dGB READ + %dGB WRITTEN ...\n", 512, 512, 512);
	for (i = 0; i < 1024; i++) {
	memcpy_AVX_4K_prefetched(pointerALIGN+512*1024*1024, pointerALIGN, 512*1024*1024);
	}
clocks4 = clock();
duration = (double) (clocks4 - clocks3 + 1);
printf("memcpy_AVX_4K_prefetched(): (%dMB block); %dMB copied in %d clocks or %.3fMB per clock\n", 512, 1024*( 512 ), (int) duration, (float)1024*( 512 )/ ((int) duration));
#endif
*/
// Benchmark memcpy() ]
//k = (((float)1000*TargetSize/(clocks2 - clocks1 + 1))); k=k>>20;
//j = (float)1000*1024*( 512 )/ ((int) durationGENERIC);
j = (float)CLOCKS_PER_SEC*1024*( 512 )/ ((int) durationGENERIC);
printf("RAM-to-RAM performance vs memcpy() ratio (bigger-the-better): %d%%\n", (int)((float)k*100/j));
	}

	free(TargetBlock);
	free(SourceBlock);
	exit(0);
}
#endif
void SearchIntoSlidingWindow(unsigned int* ShortMediumLongOFFSET, unsigned int* retIndex, unsigned int* retMatch, char* refStart,char* refEnd,char* encStart,char* encEnd){
	char* FoundAtPosition;
	unsigned int match=0;

// Too lazy to write Railgun-Reverse, it would save many ugly patches...

	// In order to avoid the unheardof slowness the 512MB may be reduced to 8MB... // --|
	char* refStartHOT = refEnd-(1024*8*128-1);                                     //   |
	char* refStartHOTTER = refEnd-(256*8*128-1);                                   //   |
	char* refStartHOTEST = refEnd-(4*8*128-1);                                     //   |
	char* refStartHOTultra = refEnd-(16-1);                                        //   |
	char* refStartCOLDERbig = refEnd-(256*1024*8*128-1);                           //   |

	*retIndex=0;
	*retMatch=0;
	*ShortMediumLongOFFSET=0;

#ifdef ReplaceBruteForceWithRailgunSwampshineBailOut

// Also, finally it is time to fix the stupid offset (blind for files smaller than the current window) stupidity for small files:
// Simply assign 'refStart' if it is within the current window i.e. between e.g. 'refEnd-(256*8*128-1)' and 'refEnd':

// Nasty bug fixed (pointer getting negative) only here, to be fixed in all the rest variants [
/*
	if ( refStart >= refEnd-(2048*8*128-1) ) refStartHOT = refStart;                //--|
	if ( refStart >= refEnd-(256*8*128-1) ) refStartHOTTER = refStart;              //--|
	if ( refStart >= refEnd-(2*4*8*128-1) ) refStartHOTEST = refStart;              //--|
	if ( refStart >= refEnd-(512*1024*8*128-1) ) refStartCOLDERbig = refStart;      //--|
                                                                                        // \ /
*/
// Nasty bug fixed (pointer getting negative) only here, to be fixed in all the rest variants ]

	if ( (1024*8*128-1)>= refEnd-refStart ) refStartHOT = refStart;                
	if ( (256*8*128-1)>= refEnd-refStart ) refStartHOTTER = refStart;              
	if ( (4*8*128-1)>= refEnd-refStart ) refStartHOTEST = refStart;              
	if ( (16-1)>= refEnd-refStart ) refStartHOTultra = refStart;                
	if ( (256*1024*8*128-1)>= refEnd-refStart ) refStartCOLDERbig = refStart;     
	//printf("%d\n", refStartCOLDERbig); //debug
	//printf("%p\n", refStartCOLDERbig); //debug

// Chosen order is sizewise:

// (3+0 +  0):1 =  3:1 priority#11    #01  7:1 =  7 (OVERRIDE) with 24:2 = 12
// (3+0 +  4):1 =  7:1 priority#01    #02 16:2 =  8
// (3+0 +  8):1 = 11:1 priority#03    #03 11:1 = 11 (OVERRIDE) with 24:3 = 8
// (3+0 + 12):1 = 15:1 priority#05    #04 12:2 =  6
// (3+1 +  0):2 =  4:2 priority#14    #05 15:1 = 15 (OVERRIDE) with 24:4 = 6
// (3+1 +  4):2 =  8:2 priority#09    #06 17:3 =  5.6
// (3+1 +  8):2 = 12:2 priority#04    #07 18:4 =  4.5 
// (3+1 + 12):2 = 16:2 priority#02    #08 13:3 =  4.3
// (3+2 +  0):3 =  5:3 priority#15    #09  8:2 =  4
// (3+2 +  4):3 =  9:3 priority#12    #10 14:4 =  3.5
// (3+2 +  8):3 = 13:3 priority#08    #11  3:1 =  3 (NOT USED)
// (3+2 + 12):3 = 17:3 priority#06    #12  9:3 =  3
// (3+3 +  0):4 =  6:4 priority#16    #13 10:4 =  2.5
// (3+3 +  4):4 = 10:4 priority#13    #14  4:2 =  2
// (3+3 +  8):4 = 14:4 priority#10    #15  5:3 =  1.6
// (3+3 + 12):4 = 18:4 priority#07    #16  6:4 =  1.5

// 24:2 = 12

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTEST >= refStart)
	if (refStartHOTEST < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTEST, encStart, (uint32_t)(refEnd-refStartHOTEST), 24);	
		if (FoundAtPosition!=NULL) {
				*retMatch=24;
				// The first four bits should be:
				                                                                  // 0100b = 4
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFF0)|0x0004; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=2;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 16:2 =  8

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTEST >= refStart)
	if (refStartHOTEST < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTEST, encStart, (uint32_t)(refEnd-refStartHOTEST), 16);	
		if (FoundAtPosition!=NULL) {
				*retMatch=16;
				// The first four bits should be:
				                                                                  // 1101b = 13
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFF0)|0x000D; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=2;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]


// 24:3 = 8

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTTER >= refStart)
	if (refStartHOTTER < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), 24);	
		if (FoundAtPosition!=NULL) {
				*retMatch=24;
				// The first four bits should be:
				                                                                    // 1000b = 8
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFF0)|0x0008; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=3;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOT >= refStart)
	if (refStartHOT < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOT, encStart, (uint32_t)(refEnd-refStartHOT), 24);	
		if (FoundAtPosition!=NULL) {
				*retMatch=24;
				// The first four bits should be:
				                                                                    // 1000b = 8
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFF0)|0x0008; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=3;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 12:2 = 8

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTEST >= refStart)
	if (refStartHOTEST < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTEST, encStart, (uint32_t)(refEnd-refStartHOTEST), 12);	
		if (FoundAtPosition!=NULL) {
				*retMatch=12;
				// The first four bits should be:
				                                                                  // 1001b = 9
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFF0)|0x0009; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=2;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 24:4 = 6

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTTER >= refStart)
	if (refStartHOTTER < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), 24);	
		if (FoundAtPosition!=NULL) {
				*retMatch=24;
				// The first four bits should be:
   				                                                                      // 1100b = 12
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x000C; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOT >= refStart)
	if (refStartHOT < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOT, encStart, (uint32_t)(refEnd-refStartHOT), 24);	
		if (FoundAtPosition!=NULL) {
				*retMatch=24;
				// The first four bits should be:
   				                                                                      // 1100b = 12
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x000C; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartCOLDERbig >= refStart)
	if (refStartCOLDERbig < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartCOLDERbig, encStart, (uint32_t)(refEnd-refStartCOLDERbig), 24);	
		if (FoundAtPosition!=NULL) {
				*retMatch=24;
				// The first four bits should be:
   				                                                                      // 1100b = 12
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x000C; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 17:3 =  5.6

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTTER >= refStart)
	if (refStartHOTTER < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), 17);	
		if (FoundAtPosition!=NULL) {
				*retMatch=17;
				// The first four bits should be:
				                                                                    // 1110b = 14
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFF0)|0x000E; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=3;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOT >= refStart)
	if (refStartHOT < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOT, encStart, (uint32_t)(refEnd-refStartHOT), 17);	
		if (FoundAtPosition!=NULL) {
				*retMatch=17;
				// The first four bits should be:
				                                                                    // 1110b = 14
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFF0)|0x000E; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=3;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 18:4 =  4.5 

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTTER >= refStart)
	if (refStartHOTTER < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), 18);	
		if (FoundAtPosition!=NULL) {
				*retMatch=18;
				// The first four bits should be:
   				                                                                      // 1111b = 15
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x000F; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOT >= refStart)
	if (refStartHOT < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOT, encStart, (uint32_t)(refEnd-refStartHOT), 18);	
		if (FoundAtPosition!=NULL) {
				*retMatch=18;
				// The first four bits should be:
   				                                                                      // 1111b = 15
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x000F; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartCOLDERbig >= refStart)
	if (refStartCOLDERbig < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartCOLDERbig, encStart, (uint32_t)(refEnd-refStartCOLDERbig), 18);	
		if (FoundAtPosition!=NULL) {
				*retMatch=18;
				// The first four bits should be:
   				                                                                      // 1111b = 15
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x000F; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 13:3 =  4.3

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTTER >= refStart)
	if (refStartHOTTER < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), 13);	
		if (FoundAtPosition!=NULL) {
				*retMatch=13;
				// The first four bits should be:
				                                                                    // 1010b = 10
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFF0)|0x000A; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=3;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOT >= refStart)
	if (refStartHOT < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOT, encStart, (uint32_t)(refEnd-refStartHOT), 13);	
		if (FoundAtPosition!=NULL) {
				*retMatch=13;
				// The first four bits should be:
				                                                                    // 1010b = 10
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFF0)|0x000A; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=3;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 8:2 =  4

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTEST >= refStart)
	if (refStartHOTEST < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTEST, encStart, (uint32_t)(refEnd-refStartHOTEST), 8);	
		if (FoundAtPosition!=NULL) {
				*retMatch=8;
				// The first four bits should be:
				                                                                  // 0101b = 5
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFF0)|0x0005; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=2;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 14:4 =  3.5

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTTER >= refStart)
	if (refStartHOTTER < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), 14);	
		if (FoundAtPosition!=NULL) {
				*retMatch=14;
				// The first four bits should be:
   				                                                                      // 1011b = 11
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x000B; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOT >= refStart)
	if (refStartHOT < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOT, encStart, (uint32_t)(refEnd-refStartHOT), 14);	
		if (FoundAtPosition!=NULL) {
				*retMatch=14;
				// The first four bits should be:
   				                                                                      // 1011b = 11
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x000B; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartCOLDERbig >= refStart)
	if (refStartCOLDERbig < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartCOLDERbig, encStart, (uint32_t)(refEnd-refStartCOLDERbig), 14);	
		if (FoundAtPosition!=NULL) {
				*retMatch=14;
				// The first four bits should be:
   				                                                                      // 1011b = 11
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x000B; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

/*
// 3:1 = 3

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTultra >= refStart)
	if (refStartHOTultra < refEnd) {
	FoundAtPosition = Railgun_Doublet (refStartHOTultra, encStart, (uint32_t)(refEnd-refStartHOTultra), 3);	
		if (FoundAtPosition!=NULL) {
				*retMatch=3;
				// The first four bits should be:
				                                                                // 0000b = 0
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xF0)|0x0000; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=1;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]
*/

// 9:3 = 3

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTTER >= refStart)
	if (refStartHOTTER < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), 9);	
		if (FoundAtPosition!=NULL) {
				*retMatch=9;
				// The first four bits should be:
				                                                                    // 0110b = 6
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFF0)|0x0006; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=3;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOT >= refStart)
	if (refStartHOT < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOT, encStart, (uint32_t)(refEnd-refStartHOT), 9);	
		if (FoundAtPosition!=NULL) {
				*retMatch=9;
				// The first four bits should be:
				                                                                    // 0110b = 6
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFF0)|0x0006; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=3;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 10:4 =  2.5

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTTER >= refStart)
	if (refStartHOTTER < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), 10);	
		if (FoundAtPosition!=NULL) {
				*retMatch=10;
				// The first four bits should be:
   				                                                                      // 0111b = 7
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x0007; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOT >= refStart)
	if (refStartHOT < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOT, encStart, (uint32_t)(refEnd-refStartHOT), 10);	
		if (FoundAtPosition!=NULL) {
				*retMatch=10;
				// The first four bits should be:
   				                                                                      // 0111b = 7
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x0007; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartCOLDERbig >= refStart)
	if (refStartCOLDERbig < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartCOLDERbig, encStart, (uint32_t)(refEnd-refStartCOLDERbig), 10);	
		if (FoundAtPosition!=NULL) {
				*retMatch=10;
				// The first four bits should be:
   				                                                                      // 0111b = 7
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x0007; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 4:2 =  2

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTEST >= refStart)
	if (refStartHOTEST < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTEST, encStart, (uint32_t)(refEnd-refStartHOTEST), 4);	
		if (FoundAtPosition!=NULL) {
				*retMatch=4;
				// The first four bits should be:
				                                                                  // 0001b = 1
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFF0)|0x0001; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=2;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 5:3 =  1.6

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTTER >= refStart)
	if (refStartHOTTER < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), 5);	
		if (FoundAtPosition!=NULL) {
				*retMatch=5;
				// The first four bits should be:
				                                                                    // 0010b = 2
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFF0)|0x0002; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=3;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOT >= refStart)
	if (refStartHOT < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOT, encStart, (uint32_t)(refEnd-refStartHOT), 5);	
		if (FoundAtPosition!=NULL) {
				*retMatch=5;
				// The first four bits should be:
				                                                                    // 0010b = 2
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFF0)|0x0002; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=3;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

// 6:4 =  1.5

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOTTER >= refStart)
	if (refStartHOTTER < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), 6);	
		if (FoundAtPosition!=NULL) {
				*retMatch=6;
				// The first four bits should be:
   				                                                                      // 0011b = 3
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x0003; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartHOT >= refStart)
	if (refStartHOT < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOT, encStart, (uint32_t)(refEnd-refStartHOT), 6);	
		if (FoundAtPosition!=NULL) {
				*retMatch=6;
				// The first four bits should be:
   				                                                                      // 0011b = 3
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x0003; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	if (refStartCOLDERbig >= refStart)
	if (refStartCOLDERbig < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartCOLDERbig, encStart, (uint32_t)(refEnd-refStartCOLDERbig), 6);	
		if (FoundAtPosition!=NULL) {
				*retMatch=6;
				// The first four bits should be:
   				                                                                      // 0011b = 3
				*retIndex=(((refEnd-FoundAtPosition)<<4)&0xFFFFFFF0)|0x0003; // xx ... x[LLOO]
				*ShortMediumLongOFFSET=4;
				return;
		}
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]

#else				
	while(refStart < refEnd){
		match=SlidingWindowVsLookAheadBuffer(refStart,refEnd,encStart,encEnd);
		if(match > *retMatch){
			*retMatch=match;
			*retIndex=refEnd-refStart;
		}
		if(*retMatch >= Min_Match_BAILOUT_Length) break;
		refStart++;
	}
#endif
}

unsigned int SlidingWindowVsLookAheadBuffer( char* refStart, char* refEnd, char* encStart,char* encEnd){
	int ret = 0;
	while(refStart[ret] == encStart[ret]){
		if(&refStart[ret] >= refEnd) break;
		if(&encStart[ret] >= encEnd) break;
		ret++;
		if(ret >= Min_Match_BAILOUT_Length) break;
	}
	return ret;
}

unsigned int NakaCompress(char* ret, char* src, unsigned int srcSize){
	unsigned int srcIndex=0;
	unsigned int retIndex=0;
	unsigned int index=0;
		unsigned int index1=0;// Tsuyo
	unsigned int match=0;
		unsigned int match1=0;// Tsuyo
	unsigned int notMatch=0;
	unsigned char* notMatchStart=NULL;
	char* refStart=NULL;
		char* refStart1=NULL;// Tsuyo
	char* encEnd=NULL;
		char* encEnd1=NULL;// Tsuyo

	int Melnitchka=0;
	char *Auberge[4] = {"|\0","/\0","-\0","\\\0"};
	int ProgressIndicator;

	unsigned int NumberOfFullLiterals=0;
	int GLOBALwindowmatchT1=0;
	int GLOBALwindowmatchT2=0;
	int GLOBALwindowmatchT3=0;
	int GLOBALwindowmatchT4=0;
	int GLOBALwindowmatchS1=0;
	int GLOBALwindowmatchS2=0;
	int GLOBALwindowmatchS3=0;
	int GLOBALwindowmatchS4=0;
	int GLOBALwindowmatchM1=0;
	int GLOBALwindowmatchM2=0;
	int GLOBALwindowmatchM3=0;
	int GLOBALwindowmatchM4=0;
	int GLOBALwindowmatchL1=0;
	int GLOBALwindowmatchL2=0;
	int GLOBALwindowmatchL3=0;
	int GLOBALwindowmatchL4=0;
	unsigned int ShortMediumLongOFFSET=0;
		unsigned int ShortMediumLongOFFSET1=0;// Tsuyo
	unsigned int NumberOfFlushLiteralsHeuristic=0;

	while(srcIndex < srcSize){
		if(srcIndex>=REF_SIZE)
			refStart=&src[srcIndex-REF_SIZE];
		else
			refStart=src;
		if(srcIndex>=srcSize-ENC_SIZE)
			encEnd=&src[srcSize];
		else
			encEnd=&src[srcIndex+ENC_SIZE];
		// Fixing the stupid 'search-beyond-end' bug:
		if(srcIndex+ENC_SIZE < srcSize) {
			SearchIntoSlidingWindow(&ShortMediumLongOFFSET,&index,&match,refStart,&src[srcIndex],&src[srcIndex],encEnd);
// Tsuyo [
	if (ShortMediumLongOFFSET != 0)
	if (srcIndex+(1) < srcSize) {
		if(srcIndex+(1)>=REF_SIZE)
			refStart1=&src[srcIndex+(1)-REF_SIZE];
		else
			refStart1=src;
		if(srcIndex+(1)>=srcSize-ENC_SIZE)
			encEnd1=&src[srcSize];
		else
			encEnd1=&src[srcIndex+(1)+ENC_SIZE];
		if(srcIndex+(1)+ENC_SIZE < srcSize) {

				SearchIntoSlidingWindow(&ShortMediumLongOFFSET1,&index1,&match1,refStart1,&src[srcIndex+(1)],&src[srcIndex+(1)],encEnd1);
	if (ShortMediumLongOFFSET1 != 0)
				if (match/ShortMediumLongOFFSET +1 >= match1/ShortMediumLongOFFSET1) { // a brash heuristic '+1+1'
				}
				else {
					match=0; // Pretend nothing to find.
				NumberOfFlushLiteralsHeuristic++;

// 1of2
			if(notMatch==0){
				notMatchStart=&ret[retIndex];
				retIndex++;
			}
			//else if (notMatch==(127-64-32)) {
			else if (notMatch==(127-64-32-16)) {
NumberOfFullLiterals++;
				//*notMatchStart=(unsigned char)((127-64-32)<<3);
				*notMatchStart=(unsigned char)((127-64-32-16)<<(4));
				*notMatchStart=(unsigned char)((127-64-32-16)<<(4)) ;//| 0x0F; // Entag it as Literal
				notMatch=0;
				notMatchStart=&ret[retIndex];
				retIndex++;
			}
			ret[retIndex]=src[srcIndex];
			retIndex++;
			notMatch++;
			srcIndex++;
			if ((srcIndex-1) % (1<<16) > srcIndex % (1<<16)) {
				ProgressIndicator = (int)( (srcIndex+1)*(float)100/(srcSize+1) );
				printf("%s; Each rotation means 64KB are encoded; Done %d%%; Compression Ratio: %.2f:1  \r", Auberge[Melnitchka++], ProgressIndicator, (float)(srcIndex) / (float)(retIndex)  );
				Melnitchka = Melnitchka & 3; // 0 1 2 3: 00 01 10 11
			}
// 2of2
			ShortMediumLongOFFSET=ShortMediumLongOFFSET1;
			index=index1;
			match=match1;

				}

		}
	}
// Tsuyo ]

			if ( ShortMediumLongOFFSET==1 && match==3 ) GLOBALwindowmatchT1++;
			if ( ShortMediumLongOFFSET==2 && match==24 ) GLOBALwindowmatchT2++;
			if ( ShortMediumLongOFFSET==3 && match==24 ) GLOBALwindowmatchT3++;
			if ( ShortMediumLongOFFSET==4 && match==24 ) GLOBALwindowmatchT4++;

			if ( ShortMediumLongOFFSET==2 && match==4 ) GLOBALwindowmatchS1++;
			if ( ShortMediumLongOFFSET==2 && match==8 ) GLOBALwindowmatchS2++;
			if ( ShortMediumLongOFFSET==2 && match==12 ) GLOBALwindowmatchS3++;
			if ( ShortMediumLongOFFSET==2 && match==16 ) GLOBALwindowmatchS4++;

			if ( ShortMediumLongOFFSET==3 && match==5 ) GLOBALwindowmatchM1++;
			if ( ShortMediumLongOFFSET==3 && match==9 ) GLOBALwindowmatchM2++;
			if ( ShortMediumLongOFFSET==3 && match==13 ) GLOBALwindowmatchM3++;
			if ( ShortMediumLongOFFSET==3 && match==17 ) GLOBALwindowmatchM4++;

			if ( ShortMediumLongOFFSET==4 && match==6 ) GLOBALwindowmatchL1++;
			if ( ShortMediumLongOFFSET==4 && match==10 ) GLOBALwindowmatchL2++;
			if ( ShortMediumLongOFFSET==4 && match==14 ) GLOBALwindowmatchL3++;
			if ( ShortMediumLongOFFSET==4 && match==18 ) GLOBALwindowmatchL4++;

		}
		else
			match=0; // Nothing to find.
		//if ( match<Min_Match_Length ) {
		//if ( match<Min_Match_Length || match<8 ) {
  		if ( match==0 ) {
			if(notMatch==0){
				notMatchStart=&ret[retIndex];
				retIndex++;
			}
			//else if (notMatch==(127-64-32)) {
			else if (notMatch==(127-64-32-16 +0)) {  // +1 because MAX 16 are copied at once
NumberOfFullLiterals++;
				//*notMatchStart=(unsigned char)((127-64-32)<<3);
				*notMatchStart=(unsigned char)((127-64-32-16 +0)<<(4-0)); // +1 because MAX 16 are copied at once
				*notMatchStart=(unsigned char)((127-64-32-16 +0)<<(4-0)) ;//| 0x0F; // Entag it as Literal
				notMatch=0;
				notMatchStart=&ret[retIndex];
				retIndex++;
			}
			ret[retIndex]=src[srcIndex];
			retIndex++;
			notMatch++;
			srcIndex++;
			if ((srcIndex-1) % (1<<16) > srcIndex % (1<<16)) {
				ProgressIndicator = (int)( (srcIndex+1)*(float)100/(srcSize+1) );
				printf("%s; Each rotation means 64KB are encoded; Done %d%%; Compression Ratio: %.2f:1  \r", Auberge[Melnitchka++], ProgressIndicator, (float)(srcIndex) / (float)(retIndex)  );
				Melnitchka = Melnitchka & 3; // 0 1 2 3: 00 01 10 11
			}
		} else {
			if(notMatch > 0){
				*notMatchStart=(unsigned char)((notMatch)<<(4-0));
				*notMatchStart=(unsigned char)((notMatch)<<(4-0)) ;//| 0x0F; // Entag it as Literal
				notMatch=0;
			}
// ---------------------| 
//                     \ /

			//ret[retIndex] = 0x80; // Assuming seventh/fifteenth bit is zero i.e. LONG MATCH i.e. Min_Match_BAILOUT_Length*4
	  		//if ( match==Min_Match_BAILOUT_Length ) ret[retIndex] = 0xC0; // 8bit&7bit set, SHORT MATCH if seventh/fifteenth bit is not zero i.e. Min_Match_BAILOUT_Length
//                     / \
// ---------------------|
/*
			ret[retIndex] = 0x01; // Assuming seventh/fifteenth bit is zero i.e. LONG MATCH i.e. Min_Match_BAILOUT_Length*4
	  		if ( match==Min_Match_BAILOUT_Length ) ret[retIndex] = 0x03; // 2bit&1bit set, LONG MATCH if 2bit is not zero i.e. Min_Match_BAILOUT_Length
*/
// No need of above, during compression we demanded lowest 2bits to be not 00.
			// 1bit+3bits+12bits:
			//ret[retIndex] = ret[retIndex] | ((match-Min_Match_Length)<<4);
			//ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x0F00)>>8);
			// 1bit+1bit+14bits:
			//ret[retIndex] = ret[retIndex] | ((match-Min_Match_Length)<<(8-(LengthBITS+1))); // No need to set the matchlength
// The fragment below is outrageously ineffective - instead of 8bit&7bit I have to use the lower TWO bits i.e. 2bit&1bit as flags, thus in decompressing one WORD can be fetched instead of two BYTE loads followed by SHR by 2.
// ---------------------| 
//                     \ /
			//ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x3F00)>>8); // 2+4+8=14
			//retIndex++;
			//ret[retIndex] = (char)((index-Min_Match_Length) & 0x00FF);
			//retIndex++;
//                     / \
// ---------------------|
			// Now the situation is like LOW:HIGH i.e. FF:3F i.e. 0x3FFF, 16bit&15bit used as flags,
			// should become LOW:HIGH i.e. FC:FF i.e. 0xFFFC, 2bit&1bit used as flags.
/*
			ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x00FF)<<2); // 6+8=14
			//ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x00FF)<<1); // 7+8=15
			retIndex++;
			ret[retIndex] = (char)(((index-Min_Match_Length) & 0x3FFF)>>6);
			//ret[retIndex] = (char)(((index-Min_Match_Length) & 0x7FFF)>>7);
			retIndex++;
*/
// No need of above, during compression we demanded lowest 2bits to be not 00, use the full 16bits and get rid of the stupid '+/-' Min_Match_Length.
			//if (index>0xFFFF) {printf ("\nFatal error: Overflow!\n"); exit(13);}
			//memcpy(&ret[retIndex],&index,2+1); // copy lower 2 bytes
			//retIndex++;
			//retIndex++;
			 //retIndex++;
			memcpy(&ret[retIndex],&index,ShortMediumLongOFFSET);
			retIndex = retIndex + ShortMediumLongOFFSET;
//                     / \
// ---------------------|
			srcIndex+=match;
			if ((srcIndex-match) % (1<<16) > srcIndex % (1<<16)) {
				ProgressIndicator = (int)( (srcIndex+1)*(float)100/(srcSize+1) );
				printf("%s; Each rotation means 64KB are encoded; Done %d%%; Compression Ratio: %.2f:1  \r", Auberge[Melnitchka++], ProgressIndicator, (float)(srcIndex) / (float)(retIndex)  );
				Melnitchka = Melnitchka & 3; // 0 1 2 3: 00 01 10 11
			}
		}
	}
	if(notMatch > 0){
		*notMatchStart=(unsigned char)((notMatch)<<(4-0));
		*notMatchStart=(unsigned char)((notMatch)<<(4-0)) ;//| 0x0F; // Entag it as Literal
	}
	printf("%s; Each rotation means 64KB are encoded; Done %d%%; Compression Ratio: %.2f:1  \n", Auberge[Melnitchka], 100, (float)(srcIndex) / (float)(retIndex) );
	printf("NumberOfFullLiterals (lower-the-better): %d\n", NumberOfFullLiterals );
	printf("NumberOfFlushLiteralsHeuristic (bigger-the-better): %d\n", NumberOfFlushLiteralsHeuristic );
	printf("Legend: WindowSizes: 1/2/3/4=Tiny/Short/Medium/Long\n");

//printf("NumberOf(Tiny)Matches[Tiny]Window (%d)[%d]: %d\n", 3, 1, GLOBALwindowmatchT1);

printf("NumberOf(Tiny)Matches[Short]Window (%d)[%d]: %d\n", 4, 2, GLOBALwindowmatchS1);
printf("NumberOf(Short)Matches[Short]Window (%d)[%d]: %d\n", 8, 2, GLOBALwindowmatchS2);
printf("NumberOf(Medium)Matches[Short]Window (%d)[%d]: %d\n", 12, 2, GLOBALwindowmatchS3);
printf("NumberOf(Long)Matches[Short]Window (%d)[%d]: %d\n", 16, 2, GLOBALwindowmatchS4);
printf("NumberOf(MaxLong)Matches[Short]Window (%d)[%d]: %d\n", 24, 2, GLOBALwindowmatchT2);

printf("NumberOf(Tiny)Matches[Medium]Window (%d)[%d]: %d\n", 5, 3, GLOBALwindowmatchM1);
printf("NumberOf(Short)Matches[Medium]Window (%d)[%d]: %d\n", 9, 3, GLOBALwindowmatchM2);
printf("NumberOf(Medium)Matches[Medium]Window (%d)[%d]: %d\n", 13, 3, GLOBALwindowmatchM3);
printf("NumberOf(Long)Matches[Medium]Window (%d)[%d]: %d\n", 17, 3, GLOBALwindowmatchM4);
printf("NumberOf(MaxLong)Matches[Medium]Window (%d)[%d]: %d\n", 24, 3, GLOBALwindowmatchT3);

printf("NumberOf(Tiny)Matches[Long]Window (%d)[%d]: %d\n", 6, 4, GLOBALwindowmatchL1);
printf("NumberOf(Short)Matches[Long]Window (%d)[%d]: %d\n", 10, 4, GLOBALwindowmatchL2);
printf("NumberOf(Medium)Matches[Long]Window (%d)[%d]: %d\n", 14, 4, GLOBALwindowmatchL3);
printf("NumberOf(Long)Matches[Long]Window (%d)[%d]: %d\n", 18, 4, GLOBALwindowmatchL4);
printf("NumberOf(MaxLong)Matches[Long]Window (%d)[%d]: %d\n", 24, 4, GLOBALwindowmatchT4);

	return retIndex;
}


uint64_t NakaDecompress (char* ret, char* src, uint64_t srcSize) {
	char* retLOCAL = ret;
	char* srcLOCAL = src;
	char* srcEndLOCAL = src+srcSize;
	unsigned int DWORDtrio;
	char* retLOCALnew;
	//uint64_t Flag; //FIX
	//uint64_t FlagMASK; //=       0xFFFFFFFFFFFFFFFF;
	//uint64_t FlagMASKnegated; //=0x0000000000000000;

	while (srcLOCAL < srcEndLOCAL) {
		DWORDtrio = *(unsigned int*)srcLOCAL;
//#ifndef _N_GP
//#ifdef _N_prefetch_4096
//		_mm_prefetch((char*)(srcLOCAL + 64*64), _MM_HINT_T0);
//#endif
//#endif
// |1stLSB    |2ndLSB  |3rdLSB   |
// -------------------------------
// |OO|LL|xxxx|xxxxxxxx|xxxxxx|xx|
// -------------------------------
// [1bit          16bit]    24bit]
// OOLL = 0000 means Literal - to ease the initial check (branchless also)
// LL = 00b means 00 MatchLength, (3+OO)+LL or 3+0 + 0|4|8|12 = 3| 7|11|15
// LL = 01b means 04 MatchLength, (3+OO)+LL or 3+1 + 0|4|8|12 = 4| 8|12|16
// LL = 10b means 08 MatchLength, (3+OO)+LL or 3+2 + 0|4|8|12 = 5| 9|13|17
// LL = 11b means 12 MatchLength, (3+OO)+LL or 3+3 + 0|4|8|12 = 6|10|14|18
// OO = 00b MatchOffset, 0xFFFFFFFF>>(3-OO), 1 bytes long i.e. Sliding Window is 1*8-LL-OO=(1+OO)*8-4=04 or   16B    
// OO = 01b MatchOffset, 0xFFFFFFFF>>(3-OO), 2 bytes long i.e. Sliding Window is 2*8-LL-OO=(1+OO)*8-4=12 or   4KB    
// OO = 10b MatchOffset, 0xFFFFFFFF>>(3-OO), 3 bytes long i.e. Sliding Window is 3*8-LL-OO=(1+OO)*8-4=20 or   1MB    
// OO = 11b MatchOffset, 0xFFFFFFFF>>(3-OO), 4 bytes long i.e. Sliding Window is 4*8-LL-OO=(1+OO)*8-4=28 or 256MB     
// (3+0 +  0):1 =  3:1 priority#11    #01  7:1 =  7 (OVERRIDE) with 24:2 = 12
// (3+0 +  4):1 =  7:1 priority#01    #02 16:2 =  8
// (3+0 +  8):1 = 11:1 priority#03    #03 11:1 = 11 (OVERRIDE) with 24:3 = 8
// (3+0 + 12):1 = 15:1 priority#05    #04 12:2 =  6
// (3+1 +  0):2 =  4:2 priority#14    #05 15:1 = 15 (OVERRIDE) with 24:4 = 6
// (3+1 +  4):2 =  8:2 priority#09    #06 17:3 =  5.6
// (3+1 +  8):2 = 12:2 priority#04    #07 18:4 =  4.5 
// (3+1 + 12):2 = 16:2 priority#02    #08 13:3 =  4.3
// (3+2 +  0):3 =  5:3 priority#15    #09  8:2 =  4
// (3+2 +  4):3 =  9:3 priority#12    #10 14:4 =  3.5
// (3+2 +  8):3 = 13:3 priority#08    #11  3:1 =  3 (NOT USED)
// (3+2 + 12):3 = 17:3 priority#06    #12  9:3 =  3
// (3+3 +  0):4 =  6:4 priority#16    #13 10:4 =  2.5
// (3+3 +  4):4 = 10:4 priority#13    #14  4:2 =  2
// (3+3 +  8):4 = 14:4 priority#10    #15  5:3 =  1.6
// (3+3 + 12):4 = 18:4 priority#07    #16  6:4 =  1.5

				#ifdef _N_Branchfull
// Branchfull [
		if ( (DWORDtrio & 0x0F) == 0x00 ) { // -------------------------------------|
				#ifdef _N_GP                                             // |
		memcpy(retLOCAL, (const char *)( (uint64_t)(srcLOCAL+1) ), 16);          // |
				#endif                                                   // |
				#ifdef _N_XMM                                            // |
//		SlowCopy128bit( (const char *)( (uint64_t)(srcLOCAL+1) ), retLOCAL );    // |
			*(uint64_t*)(retLOCAL+8*(0)) = *(uint64_t*)((srcLOCAL+1)+8*(0)); // | 
			*(uint64_t*)(retLOCAL+8*(1)) = *(uint64_t*)((srcLOCAL+1)+8*(1)); // |
				#endif                                                   // |
		retLOCAL+= ((DWORDtrio>>4)&0x0F);                                        // |
		srcLOCAL+= ((DWORDtrio>>4)&0x0F)+1;                                      // |
		} else {                                                                 //\ /
			retLOCALnew = retLOCAL +3+(DWORDtrio&0x0F); // The idea is this second instance of '(DWORDtrio&0x0F)' to be reused at once.
			if ( (DWORDtrio & 0x03) == 0x00 ) {       
				// 'DWORDtrio' has to be 'falsified' to look like the new offset - 3+1|2+1|1+1:
				DWORDtrio = DWORDtrio|((DWORDtrio & 0x0F)>>2);
				retLOCALnew = retLOCAL +24; // Total override, no mumbo-jumbo arithmetic.
			}
			DWORDtrio = DWORDtrio&( 0xFFFFFFFF >> ((3-(DWORDtrio & 0x03))<<3) );
				#ifdef _N_GP
			memcpy(retLOCAL, (const char *)( (uint64_t)(retLOCAL-(DWORDtrio>>4)) ), 24);
				#endif
				#ifdef _N_XMM
//			SlowCopy128bit( (const char *)( (uint64_t)(retLOCAL-(DWORDtrio>>4)) ), retLOCAL );
			*(uint64_t*)(retLOCAL+8*(0)) = *(uint64_t*)((retLOCAL-(DWORDtrio>>4))+8*(0)); 
			*(uint64_t*)(retLOCAL+8*(1)) = *(uint64_t*)((retLOCAL-(DWORDtrio>>4))+8*(1)); 
			*(uint64_t*)(retLOCAL+8*(2)) = *(uint64_t*)((retLOCAL-(DWORDtrio>>4))+8*(2)); 
				#endif
		srcLOCAL+= 1+(DWORDtrio&0x03); // 4|3|2|1
		//retLOCAL+= 2+(DWORDtrio&0x03) + (DWORDtrio&0x0C); // 2/3/4/5/6/7/8/9/10/11/12/13/14/15/16 // Hoshimikou
		retLOCAL= retLOCALnew; // 4/5/6, 8/9/10, 12/13/14, 16/17/18, 24 // Kintaro
		}
// Branchfull ]
				#endif
	}        
	return (uint64_t)(retLOCAL - ret);
}

// ; 'Nakamichi_Kintaro_branchfull' decompression loop, 32bit:
// ; Size in bytes: b6-28+6=148
// ; Size in instructions: 53

// ; mark_description "Intel(R) C++ Compiler XE for applications running on IA-32, Version 12.1.1.258 Build 20111011";
// ; mark_description "-O3 -QxSSE2 -D_N_XMM -D_N_prefetch_4096 -D_N_Branchfull -FAcs";
// 
// _TEXT	SEGMENT  PARA PUBLIC FLAT  'CODE'
// ;	COMDAT _Decompress
// TXTST6:
// ; -- Begin  _Decompress
// ; mark_begin;
//        ALIGN     16
// 	PUBLIC _Decompress
// _Decompress	PROC NEAR 
// ; parameter 1: 24 + esp
// ; parameter 2: 28 + esp
// ; parameter 3: 32 + esp
// .B7.1:                          ; Preds .B7.0
// 
// ;;; uint64_t Decompress (char* ret, char* src, uint64_t srcSize) {
// 
//   00000 53               push ebx                               
//   00001 55               push ebp                               
//   00002 83 ec 0c         sub esp, 12                            
//   00005 8b 54 24 1c      mov edx, DWORD PTR [28+esp]            
//   00009 8b 6c 24 20      mov ebp, DWORD PTR [32+esp]            
//   0000d 8b 4c 24 18      mov ecx, DWORD PTR [24+esp]            
// 
// ;;; 	char* retLOCAL = ret;
// 
//   00011 8b d9            mov ebx, ecx                           
// 
// ;;; 	char* srcLOCAL = src;
// ;;; 	char* srcEndLOCAL = src+srcSize;
// 
//   00013 03 ea            add ebp, edx                           
// 
// ;;; 	unsigned int DWORDtrio;
// ;;; 	char* retLOCALnew;
// ;;; 	//uint64_t Flag; //FIX
// ;;; 	//uint64_t FlagMASK; //=       0xFFFFFFFFFFFFFFFF;
// ;;; 	//uint64_t FlagMASKnegated; //=0x0000000000000000;
// ;;; 
// ;;; 	while (srcLOCAL < srcEndLOCAL) {
// 
//   00015 3b d5            cmp edx, ebp                           
//   00017 0f 83 aa 00 00 
//         00               jae .B7.10 ; Prob 10%                  
//                                 ; LOE edx ecx ebx ebp esi edi
// .B7.2:                          ; Preds .B7.1
//   0001d 89 6c 24 08      mov DWORD PTR [8+esp], ebp             ;
//   00021 89 34 24         mov DWORD PTR [esp], esi               ;
//   00024 89 7c 24 04      mov DWORD PTR [4+esp], edi             ;
//                                 ; LOE edx ebx
// .B7.3:                          ; Preds .B7.8 .B7.2
// 
// ;;; 		DWORDtrio = *(unsigned int*)srcLOCAL;
// 
//   00028 8b 3a            mov edi, DWORD PTR [edx]               
// 
// ;;; //#ifndef _N_GP
// ;;; //#ifdef _N_prefetch_4096
// ;;; //		_mm_prefetch((char*)(srcLOCAL + 64*64), _MM_HINT_T0);
// ;;; //#endif
// ;;; //#endif
// ;;; // |1stLSB    |2ndLSB  |3rdLSB   |
// ;;; // -------------------------------
// ;;; // |OO|LL|xxxx|xxxxxxxx|xxxxxx|xx|
// ;;; // -------------------------------
// ;;; // [1bit          16bit]    24bit]
// ;;; // OOLL = 0000 means Literal - to ease the initial check (branchless also)
// ;;; // LL = 00b means 00 MatchLength, (3+OO)+LL or 3+0 + 0|4|8|12 = 3| 7|11|15
// ;;; // LL = 01b means 04 MatchLength, (3+OO)+LL or 3+1 + 0|4|8|12 = 4| 8|12|16
// ;;; // LL = 10b means 08 MatchLength, (3+OO)+LL or 3+2 + 0|4|8|12 = 5| 9|13|17
// ;;; // LL = 11b means 12 MatchLength, (3+OO)+LL or 3+3 + 0|4|8|12 = 6|10|14|18
// ;;; // OO = 00b MatchOffset, 0xFFFFFFFF>>(3-OO), 1 bytes long i.e. Sliding Window is 1*8-LL-OO=(1+OO)*8-4=04 or   16B    
// ;;; // OO = 01b MatchOffset, 0xFFFFFFFF>>(3-OO), 2 bytes long i.e. Sliding Window is 2*8-LL-OO=(1+OO)*8-4=12 or   4KB    
// ;;; // OO = 10b MatchOffset, 0xFFFFFFFF>>(3-OO), 3 bytes long i.e. Sliding Window is 3*8-LL-OO=(1+OO)*8-4=20 or   1MB    
// ;;; // OO = 11b MatchOffset, 0xFFFFFFFF>>(3-OO), 4 bytes long i.e. Sliding Window is 4*8-LL-OO=(1+OO)*8-4=28 or 256MB     
// ;;; // (3+0 +  0):1 =  3:1 priority#11    #01  7:1 =  7 (OVERRIDE) with 24:2 = 12
// ;;; // (3+0 +  4):1 =  7:1 priority#01    #02 16:2 =  8
// ;;; // (3+0 +  8):1 = 11:1 priority#03    #03 11:1 = 11 (OVERRIDE) with 24:3 = 8
// ;;; // (3+0 + 12):1 = 15:1 priority#05    #04 12:2 =  6
// ;;; // (3+1 +  0):2 =  4:2 priority#14    #05 15:1 = 15 (OVERRIDE) with 24:4 = 6
// ;;; // (3+1 +  4):2 =  8:2 priority#09    #06 17:3 =  5.6
// ;;; // (3+1 +  8):2 = 12:2 priority#04    #07 18:4 =  4.5 
// ;;; // (3+1 + 12):2 = 16:2 priority#02    #08 13:3 =  4.3
// ;;; // (3+2 +  0):3 =  5:3 priority#15    #09  8:2 =  4
// ;;; // (3+2 +  4):3 =  9:3 priority#12    #10 14:4 =  3.5
// ;;; // (3+2 +  8):3 = 13:3 priority#08    #11  3:1 =  3 (NOT USED)
// ;;; // (3+2 + 12):3 = 17:3 priority#06    #12  9:3 =  3
// ;;; // (3+3 +  0):4 =  6:4 priority#16    #13 10:4 =  2.5
// ;;; // (3+3 +  4):4 = 10:4 priority#13    #14  4:2 =  2
// ;;; // (3+3 +  8):4 = 14:4 priority#10    #15  5:3 =  1.6
// ;;; // (3+3 + 12):4 = 18:4 priority#07    #16  6:4 =  1.5
// ;;; 
// ;;; 				#ifdef _N_Branchfull
// ;;; // Branchfull [
// ;;; 		if ( (DWORDtrio & 0x0F) == 0x00 ) { // -------------------------------------|
// 
//   0002a 8b ef            mov ebp, edi                           
//   0002c 83 e5 0f         and ebp, 15                            
//   0002f 75 25            jne .B7.5 ; Prob 50%                   
//                                 ; LOE edx ebx ebp edi
// .B7.4:                          ; Preds .B7.3
// 
// ;;; 				#ifdef _N_GP                                             // |
// ;;; 		memcpy(retLOCAL, (const char *)( (uint64_t)(srcLOCAL+1) ), 16);          // |
// ;;; 				#endif                                                   // |
// ;;; 				#ifdef _N_XMM                                            // |
// ;;; //		SlowCopy128bit( (const char *)( (uint64_t)(srcLOCAL+1) ), retLOCAL );    // |
// ;;; 			*(uint64_t*)(retLOCAL+8*(0)) = *(uint64_t*)((srcLOCAL+1)+8*(0)); // | 
// ;;; 			*(uint64_t*)(retLOCAL+8*(1)) = *(uint64_t*)((srcLOCAL+1)+8*(1)); // |
// ;;; 				#endif                                                   // |
// ;;; 		retLOCAL+= ((DWORDtrio>>4)&0x0F);                                        // |
// 
//   00031 c1 ef 04         shr edi, 4                             
//   00034 83 e7 0f         and edi, 15                            
//   00037 8b 4a 01         mov ecx, DWORD PTR [1+edx]             
//   0003a 89 0b            mov DWORD PTR [ebx], ecx               
//   0003c 8b 6a 05         mov ebp, DWORD PTR [5+edx]             
//   0003f 8b 72 09         mov esi, DWORD PTR [9+edx]             
//   00042 8b 4a 0d         mov ecx, DWORD PTR [13+edx]            
// 
// ;;; 		srcLOCAL+= ((DWORDtrio>>4)&0x0F)+1;                                      // |
// 
//   00045 8d 54 17 01      lea edx, DWORD PTR [1+edi+edx]         
//   00049 89 6b 04         mov DWORD PTR [4+ebx], ebp             
//   0004c 89 73 08         mov DWORD PTR [8+ebx], esi             
//   0004f 89 4b 0c         mov DWORD PTR [12+ebx], ecx            
//   00052 03 df            add ebx, edi                           
//   00054 eb 5c            jmp .B7.8 ; Prob 100%                  
//                                 ; LOE edx ebx
// .B7.5:                          ; Preds .B7.3
// 
// ;;; 		} else {                                                                 //\ /
// ;;; 			retLOCALnew = retLOCAL +3+(DWORDtrio&0x0F); // The idea is this second instance of '(DWORDtrio&0x0F)' to be reused at once.
// ;;; 			if ( (DWORDtrio & 0x03) == 0x00 ) {       
// 
//   00056 8b cf            mov ecx, edi                           
//   00058 8d 74 2b 03      lea esi, DWORD PTR [3+ebx+ebp]         
//   0005c 83 e1 03         and ecx, 3                             
//   0005f 75 0d            jne .B7.7 ; Prob 50%                   
//                                 ; LOE edx ecx ebx ebp esi edi
// .B7.6:                          ; Preds .B7.5
// 
// ;;; 				// 'DWORDtrio' has to be 'falsified' to look like the new offset - 3+1|2+1|1+1:
// ;;; 				DWORDtrio = DWORDtrio|((DWORDtrio & 0x0F)>>2);
// 
//   00061 c1 ed 02         shr ebp, 2                             
// 
// ;;; 				retLOCALnew = retLOCAL +24; // Total override, no mumbo-jumbo arithmetic.
// 
//   00064 8d 73 18         lea esi, DWORD PTR [24+ebx]            
//   00067 0b fd            or edi, ebp                            
// 
// ;;; 			}
// ;;; 			DWORDtrio = DWORDtrio&( 0xFFFFFFFF >> ((3-(DWORDtrio & 0x03))<<3) );
// 
//   00069 8b cf            mov ecx, edi                           
//   0006b 83 e1 03         and ecx, 3                             
//                                 ; LOE edx ecx ebx esi edi
// .B7.7:                          ; Preds .B7.6 .B7.5
//   0006e 83 f1 03         xor ecx, 3                             
//   00071 bd ff ff ff ff   mov ebp, -1                            
//   00076 c1 e1 03         shl ecx, 3                             
//   00079 d3 ed            shr ebp, cl                            
//   0007b 23 fd            and edi, ebp                           
// 
// ;;; 				#ifdef _N_GP
// ;;; 			memcpy(retLOCAL, (const char *)( (uint64_t)(retLOCAL-(DWORDtrio>>4)) ), 24);
// ;;; 				#endif
// ;;; 				#ifdef _N_XMM
// ;;; //			SlowCopy128bit( (const char *)( (uint64_t)(retLOCAL-(DWORDtrio>>4)) ), retLOCAL );
// ;;; 			*(uint64_t*)(retLOCAL+8*(0)) = *(uint64_t*)((retLOCAL-(DWORDtrio>>4))+8*(0)); 
// 
//   0007d 8b ef            mov ebp, edi                           
// 
// ;;; 			*(uint64_t*)(retLOCAL+8*(1)) = *(uint64_t*)((retLOCAL-(DWORDtrio>>4))+8*(1)); 
// ;;; 			*(uint64_t*)(retLOCAL+8*(2)) = *(uint64_t*)((retLOCAL-(DWORDtrio>>4))+8*(2)); 
// ;;; 				#endif
// ;;; 		srcLOCAL+= 1+(DWORDtrio&0x03); // 4|3|2|1
// 
//   0007f 83 e7 03         and edi, 3                             
//   00082 c1 ed 04         shr ebp, 4                             
//   00085 f7 dd            neg ebp                                
//   00087 03 eb            add ebp, ebx                           
//   00089 8d 54 3a 01      lea edx, DWORD PTR [1+edx+edi]         
//   0008d 8b 4d 00         mov ecx, DWORD PTR [ebp]               
//   00090 8b 45 04         mov eax, DWORD PTR [4+ebp]             
//   00093 89 0b            mov DWORD PTR [ebx], ecx               
//   00095 89 43 04         mov DWORD PTR [4+ebx], eax             
//   00098 8b 4d 08         mov ecx, DWORD PTR [8+ebp]             
//   0009b 8b 45 0c         mov eax, DWORD PTR [12+ebp]            
//   0009e 89 4b 08         mov DWORD PTR [8+ebx], ecx             
//   000a1 89 43 0c         mov DWORD PTR [12+ebx], eax            
//   000a4 8b 4d 10         mov ecx, DWORD PTR [16+ebp]            
//   000a7 8b 6d 14         mov ebp, DWORD PTR [20+ebp]            
//   000aa 89 4b 10         mov DWORD PTR [16+ebx], ecx            
//   000ad 89 6b 14         mov DWORD PTR [20+ebx], ebp            
// 
// ;;; 		//retLOCAL+= 2+(DWORDtrio&0x03) + (DWORDtrio&0x0C); // 2/3/4/5/6/7/8/9/10/11/12/13/14/15/16 // Hoshimikou
// ;;; 		retLOCAL= retLOCALnew; // 4/5/6, 8/9/10, 12/13/14, 16/17/18, 24 // Kintaro
// 
//   000b0 8b de            mov ebx, esi                           
//                                 ; LOE edx ebx
// .B7.8:                          ; Preds .B7.4 .B7.7
//   000b2 3b 54 24 08      cmp edx, DWORD PTR [8+esp]             
//   000b6 0f 82 6c ff ff 
//         ff               jb .B7.3 ; Prob 82%                    
//                                 ; LOE edx ebx
// .B7.9:                          ; Preds .B7.8
//   000bc 8b 4c 24 18      mov ecx, DWORD PTR [24+esp]            ;
//   000c0 8b 34 24         mov esi, DWORD PTR [esp]               ;
//   000c3 8b 7c 24 04      mov edi, DWORD PTR [4+esp]             ;
//                                 ; LOE ecx ebx esi edi
// .B7.10:                         ; Preds .B7.9 .B7.1
// 
// ;;; 		}
// ;;; // Branchfull ]
// ;;; 				#endif
// ;;; 	}        
// ;;; 	return (uint64_t)(retLOCAL - ret);
// 
//   000c7 8b c3            mov eax, ebx                           
//   000c9 2b d9            sub ebx, ecx                           
//   000cb 99               cdq                                    
//   000cc 8b c1            mov eax, ecx                           
//   000ce 8b ea            mov ebp, edx                           
//   000d0 99               cdq                                    
//   000d1 1b ea            sbb ebp, edx                           
//   000d3 8b c3            mov eax, ebx                           
//   000d5 8b d5            mov edx, ebp                           
//   000d7 83 c4 0c         add esp, 12                            
//   000da 5d               pop ebp                                
//   000db 5b               pop ebx                                
//   000dc c3               ret                                    
//   000dd 0f 1f 00         ALIGN     16
//                                 ; LOE
// ; mark_end;
// _Decompress ENDP
// ;_Decompress	ENDS
// _TEXT	ENDS
// _DATA	SEGMENT  DWORD PUBLIC FLAT  'DATA'
// _DATA	ENDS
// ; -- End  _Decompress

/*
; 'Nakamichi_Shifune_branchfull' decompression loop:
; Size in bytes: 93-15+2=128
; Size in instructions: 38
; mark_description "Intel(R) C++ Intel(R) 64 Compiler XE for applications running on Intel(R) 64, Version 15.0.0.108 Build 20140";
; mark_description "-O3 -QxSSE2 -D_N_XMM -D_N_prefetch_4096 -D_N_Branchfull -FAcs";

.B7.3::                         
  00015 45 8b 1a         mov r11d, DWORD PTR [r10]              
  00018 41 f7 c3 0f 00 
        00 00            test r11d, 15                          
  0001f 74 4d            je .B7.5 
.B7.4::                         
  00021 44 89 d9         mov ecx, r11d                          
  00024 ba ff ff ff ff   mov edx, -1                            
  00029 83 f1 03         xor ecx, 3                             
  0002c c1 e1 03         shl ecx, 3                             
  0002f d3 ea            shr edx, cl                            
  00031 44 23 da         and r11d, edx                          
  00034 44 89 da         mov edx, r11d                          
  00037 c1 ea 04         shr edx, 4                             
  0003a 48 f7 da         neg rdx                                
  0003d 48 03 d0         add rdx, rax                           
  00040 48 8b 0a         mov rcx, QWORD PTR [rdx]               
  00043 48 89 08         mov QWORD PTR [rax], rcx               
  00046 48 8b 4a 08      mov rcx, QWORD PTR [8+rdx]             
  0004a 48 89 48 08      mov QWORD PTR [8+rax], rcx             
  0004e 44 89 d9         mov ecx, r11d                          
  00051 83 e1 03         and ecx, 3                             
  00054 41 83 e3 0f      and r11d, 15                           
  00058 ff c1            inc ecx                                
  0005a 41 83 c3 03      add r11d, 3                            
  0005e 48 8b 52 10      mov rdx, QWORD PTR [16+rdx]            
  00062 4c 03 d1         add r10, rcx                           
  00065 48 89 50 10      mov QWORD PTR [16+rax], rdx            
  00069 49 03 c3         add rax, r11                           
  0006c eb 22            jmp .B7.6 
.B7.5::                         
  0006e 41 c1 eb 04      shr r11d, 4                            
  00072 44 89 da         mov edx, r11d                          
  00075 41 83 e3 0f      and r11d, 15                           
  00079 f3 41 0f 6f 42 
        01               movdqu xmm0, XMMWORD PTR [1+r10]       
  0007f 41 ff c3         inc r11d                               
  00082 48 83 e2 0f      and rdx, 15                            
  00086 f3 0f 7f 00      movdqu XMMWORD PTR [rax], xmm0         
  0008a 48 03 c2         add rax, rdx                           
  0008d 4d 03 d3         add r10, r11                           
.B7.6::                         
  00090 4d 3b d0         cmp r10, r8                            
  00093 72 80            jb .B7.3 
*/

/*
; 'Nakamichi_Mustang_branchfull' decompression loop:
; Size in bytes: 7c-14+2=106
; Size in instructions: 34
; mark_description "Intel(R) C++ Intel(R) 64 Compiler XE for applications running on Intel(R) 64, Version 15.0.0.108 Build 20140";
; mark_description "-O3 -QxSSE2 -D_N_XMM -D_N_prefetch_4096 -D_N_Branchfull -FAcs";

.B7.3::                         
  00014 41 8b 11         mov edx, DWORD PTR [r9]                
  00017 f6 c2 0f         test dl, 15                            
  0001a 75 20            jne .B7.5 
.B7.4::                         
  0001c c1 ea 04         shr edx, 4                             
  0001f 89 d1            mov ecx, edx                           
  00021 83 e2 0f         and edx, 15                            
  00024 f3 41 0f 6f 41 
        01               movdqu xmm0, XMMWORD PTR [1+r9]        
  0002a ff c2            inc edx                                
  0002c 48 83 e1 0f      and rcx, 15                            
  00030 f3 0f 7f 00      movdqu XMMWORD PTR [rax], xmm0         
  00034 48 03 c1         add rax, rcx                           
  00037 4c 03 ca         add r9, rdx                            
  0003a eb 3d            jmp .B7.6 
.B7.5::                         
  0003c 89 d1            mov ecx, edx                           
  0003e 41 bb ff ff ff 
        ff               mov r11d, -1                           
  00044 83 f1 03         xor ecx, 3                             
  00047 c1 e1 03         shl ecx, 3                             
  0004a 41 d3 eb         shr r11d, cl                           
  0004d 41 23 d3         and edx, r11d                          
  00050 89 d1            mov ecx, edx                           
  00052 41 89 d3         mov r11d, edx                          
  00055 c1 e9 04         shr ecx, 4                             
  00058 41 83 e3 03      and r11d, 3                            
  0005c 83 e2 0f         and edx, 15                            
  0005f 48 f7 d9         neg rcx                                
  00062 41 ff c3         inc r11d                               
  00065 48 03 c8         add rcx, rax                           
  00068 83 c2 02         add edx, 2                             
  0006b 4d 03 cb         add r9, r11                            
  0006e f3 0f 6f 01      movdqu xmm0, XMMWORD PTR [rcx]         
  00072 f3 0f 7f 00      movdqu XMMWORD PTR [rax], xmm0         
  00076 48 03 c2         add rax, rdx                           
.B7.6::                         
  00079 4d 3b ca         cmp r9, r10                            
  0007c 72 96            jb .B7.3 
*/

/*
; 'Nakamichi_Mustang_branchless' decompression loop:
; Size in bytes: bc-3a+6=136
; Size in instructions: 40
; mark_description "Intel(R) C++ Intel(R) 64 Compiler XE for applications running on Intel(R) 64, Version 15.0.0.108 Build 20140";
; mark_description "-O3 -QxSSE2 -D_N_XMM -D_N_prefetch_4096 -D_N_Branchless -FAcs";

.B7.3::                         
  0003a 45 8b 30         mov r14d, DWORD PTR [r8]               
  0003d 44 89 f1         mov ecx, r14d                          
  00040 83 f1 03         xor ecx, 3                             
  00043 41 bf ff ff ff 
        ff               mov r15d, -1                           
  00049 c1 e1 03         shl ecx, 3                             
  0004c 45 33 ed         xor r13d, r13d                         
  0004f 41 f7 c6 0f 00 
        00 00            test r14d, 15                          
  00056 44 0f 44 ea      cmove r13d, edx                        
  0005a 41 d3 ef         shr r15d, cl                           
  0005d 48 89 c1         mov rcx, rax                           
  00060 45 23 f7         and r14d, r15d                         
  00063 4d 89 c7         mov r15, r8                            
  00066 44 89 f5         mov ebp, r14d                          
  00069 c1 ed 04         shr ebp, 4                             
  0006c 49 ff cd         dec r13                                
  0006f 41 89 eb         mov r11d, ebp                          
  00072 4d 89 ec         mov r12, r13                           
  00075 49 2b cb         sub rcx, r11                           
  00078 49 f7 d4         not r12                                
  0007b 48 ff c9         dec rcx                                
  0007e 4d 23 fc         and r15, r12                           
  00081 49 23 cd         and rcx, r13                           
  00084 ff c5            inc ebp                                
  00086 4d 23 dc         and r11, r12                           
  00089 49 23 ec         and rbp, r12                           
  0008c f3 42 0f 6f 44 
        39 01            movdqu xmm0, XMMWORD PTR [1+rcx+r15]   
  00093 44 89 f1         mov ecx, r14d                          
  00096 41 83 e6 03      and r14d, 3                            
  0009a 83 e1 0f         and ecx, 15                            
  0009d 41 ff c6         inc r14d                               
  000a0 83 c1 02         add ecx, 2                             
  000a3 4d 23 f5         and r14, r13                           
  000a6 49 23 cd         and rcx, r13                           
  000a9 49 03 ee         add rbp, r14                           
  000ac 4c 03 d9         add r11, rcx                           
  000af 4c 03 c5         add r8, rbp                            
  000b2 f3 0f 7f 00      movdqu XMMWORD PTR [rax], xmm0         
  000b6 49 03 c3         add rax, r11                           
  000b9 4d 3b c2         cmp r8, r10                            
  000bc 0f 82 78 ff ff 
        ff               jb .B7.3 
*/

// Results on Core2 T7500 2200MHz:
/*
D:\Nakamichi_Rakka>Nakamichi_Rakka_YMMless.exe enwik8.Nakamichi /report
Nakamichi 'Rakka', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 41699410 bytes ...
RAM-to-RAM performance: 448 MB/s.
Memory pool starting address: 0000000002C40080 ... 64 byte aligned, OK
Copying a 256MB block 1024 times i.e. 256GB READ + 256GB WRITTEN ...
memcpy(): (256MB block); 262144MB copied in 139621 clocks or 1.878MB per clock
RAM-to-RAM performance vs memcpy() ratio (bigger-the-better): 23%

D:\Nakamichi_Rakka>Nakamichi_Rakka_YMMless.exe OSHO.TXT.Nakamichi /report
Nakamichi 'Rakka', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 66989899 bytes ...
RAM-to-RAM performance: 576 MB/s.
Memory pool starting address: 0000000004450080 ... 64 byte aligned, OK
Copying a 256MB block 1024 times i.e. 256GB READ + 256GB WRITTEN ...
memcpy(): (256MB block); 262144MB copied in 139559 clocks or 1.878MB per clock
RAM-to-RAM performance vs memcpy() ratio (bigger-the-better): 30%

D:\Nakamichi_Rakka>Nakamichi_Rakka_YMMless.exe silesia.tar
Nakamichi 'Rakka', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Compressing 211948544 bytes ...
-; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 457929
NumberOf(Tiny)Matches[Tiny]Window (4): 501239
NumberOf(Short)Matches[Tiny]Window (8): 175044
NumberOf(Short)Matches[Long]Window (8): 436335
NumberOf(Medium)Matches[Long]Window (16): 185722
NumberOf(Long)Matches[Long]Window (32): 113643
RAM-to-RAM performance: 2 KB/s.

05/16/2014  08:22 AM       206,908,949 OSHO.TXT
09/15/2014  08:18 AM        66,989,899 OSHO.TXT.Rakka.Nakamichi

05/16/2014  08:22 AM       211,948,544 silesia.tar
09/16/2014  08:58 PM        80,315,189 silesia.tar.Rakka.Nakamichi

D:\Nakamichi_Rakka>Nakamichi_Rakka_YMMless.exe silesia.tar.Nakamichi /report
Nakamichi 'Rakka', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced, muffinesque suggestion by Jim Dempsey enforced.
Decompressing 80315189 bytes ...
RAM-to-RAM performance: 512 MB/s.
*/


// Railgun_Swampshine_BailOut, copyleft 2014-Apr-27, Kaze.
// 2014-Apr-27: The nasty SIGNED/UNSIGNED bug in 'Swampshines' which I illustrated several months ago in my fuzzy search article now is fixed here too:
/*
The bug is this (the variables 'i' and 'PRIMALposition' are uint32_t):
Next line assumes -19 >= 0 is true:
if ( (i-(PRIMALposition-1)) >= 0) printf ("THE NASTY BUG AGAIN: %d >= 0\n", i-(PRIMALposition-1));
Next line assumes -19 >= 0 is false:
if ( (signed int)(i-(PRIMALposition-1)) >= 0) printf ("THE NASTY BUG AGAIN: %d >= 0\n", i-(PRIMALposition-1));
And the actual fix:
...
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4
// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {
...
*/
// Railgun_Swampshine_BailOut, copyleft 2014-Jan-31, Kaze.
// Caution: For better speed the case 'if (cbPattern==1)' was removed, so Pattern must be longer than 1 char.
#define NeedleThreshold2vs4swampLITE 9+10 // Should be bigger than 9. BMH2 works up to this value (inclusive), if bigger then BMH4 takes over.
char * Railgun_Swampshine_BailOut (char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern)
{
	char * pbTargetMax = pbTarget + cbTarget;
	register uint32_t ulHashPattern;
	signed long count;

	unsigned char bm_Horspool_Order2[256*256]; // Bitwise soon...
	uint32_t i, Gulliver;

	uint32_t PRIMALposition, PRIMALpositionCANDIDATE;
	uint32_t PRIMALlength, PRIMALlengthCANDIDATE;
	uint32_t j, FoundAtPosition;

	if (cbPattern > cbTarget) return(NULL);

	if ( cbPattern<4 ) { 
		// SSE2 i.e. 128bit Assembly rules here:
		// ...
        	pbTarget = pbTarget+cbPattern;
		ulHashPattern = ( (*(char *)(pbPattern))<<8 ) + *(pbPattern+(cbPattern-1));
		if ( cbPattern==3 ) {
			for ( ;; ) {
				if ( ulHashPattern == ( (*(char *)(pbTarget-3))<<8 ) + *(pbTarget-1) ) {
					if ( *(char *)(pbPattern+1) == *(char *)(pbTarget-2) ) return((pbTarget-3));
				}
				if ( (char)(ulHashPattern>>8) != *(pbTarget-2) ) { 
					pbTarget++;
					if ( (char)(ulHashPattern>>8) != *(pbTarget-2) ) pbTarget++;
				}
				pbTarget++;
				if (pbTarget > pbTargetMax) return(NULL);
			}
		} else {
		}
		for ( ;; ) {
			if ( ulHashPattern == ( (*(char *)(pbTarget-2))<<8 ) + *(pbTarget-1) ) return((pbTarget-2));
			if ( (char)(ulHashPattern>>8) != *(pbTarget-1) ) pbTarget++;
			pbTarget++;
			if (pbTarget > pbTargetMax) return(NULL);
		}
	} else { //if ( cbPattern<4 )
		if ( cbPattern<=NeedleThreshold2vs4swampLITE ) { 
			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
							if ( count <= 0 ) return(pbTarget+i);
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);
		} else { // if ( cbPattern<=NeedleThreshold2vs4swampLITE )

// Swampwalker_BAILOUT heuristic order 4 (Needle should be bigger than 4) [
// Needle: 1234567890qwertyuiopasdfghjklzxcv            PRIMALposition=01 PRIMALlength=33  '1234567890qwertyuiopasdfghjklzxcv'
// Needle: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv            PRIMALposition=29 PRIMALlength=04  'vvvv'
// Needle: vvvvvvvvvvBOOMSHAKALAKAvvvvvvvvvv            PRIMALposition=08 PRIMALlength=20  'vvvBOOMSHAKALAKAvvvv'
// Needle: Trollland                                    PRIMALposition=01 PRIMALlength=09  'Trollland'
// Needle: Swampwalker                                  PRIMALposition=01 PRIMALlength=11  'Swampwalker'
// Needle: licenselessness                              PRIMALposition=01 PRIMALlength=15  'licenselessness'
// Needle: alfalfa                                      PRIMALposition=02 PRIMALlength=06  'lfalfa'
// Needle: Sandokan                                     PRIMALposition=01 PRIMALlength=08  'Sandokan'
// Needle: shazamish                                    PRIMALposition=01 PRIMALlength=09  'shazamish'
// Needle: Simplicius Simplicissimus                    PRIMALposition=06 PRIMALlength=20  'icius Simplicissimus'
// Needle: domilliaquadringenquattuorquinquagintillion  PRIMALposition=01 PRIMALlength=32  'domilliaquadringenquattuorquinqu'
// Needle: boom-boom                                    PRIMALposition=02 PRIMALlength=08  'oom-boom'
// Needle: vvvvv                                        PRIMALposition=01 PRIMALlength=04  'vvvv'
// Needle: 12345                                        PRIMALposition=01 PRIMALlength=05  '12345'
// Needle: likey-likey                                  PRIMALposition=03 PRIMALlength=09  'key-likey'
// Needle: BOOOOOM                                      PRIMALposition=03 PRIMALlength=05  'OOOOM'
// Needle: aaaaaBOOOOOM                                 PRIMALposition=02 PRIMALlength=09  'aaaaBOOOO'
// Needle: BOOOOOMaaaaa                                 PRIMALposition=03 PRIMALlength=09  'OOOOMaaaa'
PRIMALlength=0;
for (i=0+(1); i < cbPattern-((4)-1)+(1)-(1); i++) { // -(1) because the last BB order 4 has no counterpart(s)
	FoundAtPosition = cbPattern - ((4)-1) + 1;
	PRIMALpositionCANDIDATE=i;
	while ( PRIMALpositionCANDIDATE <= (FoundAtPosition-1) ) {
		j = PRIMALpositionCANDIDATE + 1;
		while ( j <= (FoundAtPosition-1) ) {
			if ( *(uint32_t *)(pbPattern+PRIMALpositionCANDIDATE-(1)) == *(uint32_t *)(pbPattern+j-(1)) ) FoundAtPosition = j;
			j++;
		}
		PRIMALpositionCANDIDATE++;
	}
	PRIMALlengthCANDIDATE = (FoundAtPosition-1)-i+1 +((4)-1);
	if (PRIMALlengthCANDIDATE >= PRIMALlength) {PRIMALposition=i; PRIMALlength = PRIMALlengthCANDIDATE;}
	if (cbPattern-i+1 <= PRIMALlength) break;
	if (PRIMALlength > 128) break; // Bail Out for 129[+]
}
// Swampwalker_BAILOUT heuristic order 4 (Needle should be bigger than 4) ]

// Here we have 4 or bigger NewNeedle, apply order 2 for pbPattern[i+(PRIMALposition-1)] with length 'PRIMALlength' and compare the pbPattern[i] with length 'cbPattern':
PRIMALlengthCANDIDATE = cbPattern;
cbPattern = PRIMALlength;
pbPattern = pbPattern + (PRIMALposition-1);

// Revision 2 commented section [
/*
if (cbPattern-1 <= 255) {
// BMH Order 2 [
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]= cbPattern-1;} // cbPattern-(Order-1) for Horspool; 'memset' if not optimized
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=i; // Rightmost appearance/position is needed
			i=0;
			while (i <= cbTarget-cbPattern) { 
				Gulliver = bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]];
				if ( Gulliver != cbPattern-1 ) { // CASE #2: if equal means the pair (char order 2) is not found i.e. Gulliver remains intact, skip the whole pattern and fall back (Order-1) chars i.e. one char for Order 2
				if ( Gulliver == cbPattern-2 ) { // CASE #1: means the pair (char order 2) is found
					if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) {
						count = cbPattern-4+1; 
						while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
							count = count-4;
// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}
					}
					Gulliver = 1;
				} else
					Gulliver = cbPattern - Gulliver - 2; // CASE #3: the pair is found and not as suffix i.e. rightmost position
				}
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);
// BMH Order 2 ]
} else {
			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);
}
*/
// Revision 2 commented section ]

		if ( cbPattern<=NeedleThreshold2vs4swampLITE ) { 

			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);

		} else { // if ( cbPattern<=NeedleThreshold2vs4swampLITE )

			// BMH pseudo-order 4, needle should be >=8+2:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			// In line below we "hash" 4bytes to 2bytes i.e. 16bit table, how to compute TOTAL number of BBs, 'cbPattern - Order + 1' is the number of BBs for text 'cbPattern' bytes long, for example, for cbPattern=11 'fastest fox' and Order=4 we have BBs = 11-4+1=8:
			//"fast"
			//"aste"
			//"stes"
			//"test"
			//"est "
			//"st f"
			//"t fo"
			//" fox"
			//for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( *(unsigned short *)(pbPattern+i+0) + *(unsigned short *)(pbPattern+i+2) ) & ( (1<<16)-1 )]=1;
			//for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( (*(uint32_t *)(pbPattern+i+0)>>16)+(*(uint32_t *)(pbPattern+i+0)&0xFFFF) ) & ( (1<<16)-1 )]=1;
			// Above line is replaced by next one with better hashing:
			for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( (*(uint32_t *)(pbPattern+i+0)>>(16-1))+(*(uint32_t *)(pbPattern+i+0)&0xFFFF) ) & ( (1<<16)-1 )]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1;
				//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]&0xFFFF) ) & ( (1<<16)-1 )] != 0 ) { // DWORD #1
				// Above line is replaced by next one with better hashing:
				if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]&0xFFFF) ) & ( (1<<16)-1 )] != 0 ) { // DWORD #1
					//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = cbPattern-(2-1)-2-4; else {
					// Above line is replaced in order to strengthen the skip by checking the middle DWORD,if the two DWORDs are 'ab' and 'cd' i.e. [2x][2a][2b][2c][2d] then the middle DWORD is 'bc'.
					// The respective offsets (backwards) are: -10/-8/-6/-4 for 'xa'/'ab'/'bc'/'cd'.
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-4-2; else {
					// Above line is replaced by next one with better hashing:
					// When using (16-1) right shifting instead of 16 we will have two different pairs (if they are equal), the highest bit being lost do the job especialy for ASCII texts with no symbols in range 128-255.
					// Example for genomesque pair TT+TT being shifted by (16-1):
					// T            = 01010100
					// TT           = 01010100 01010100
					// TTTT         = 01010100 01010100 01010100 01010100
					// TTTT>>16     = 00000000 00000000 01010100 01010100
					// TTTT>>(16-1) = 00000000 00000000 10101000 10101000 <--- Due to the left shift by 1, the 8th bits of 1st and 2nd bytes are populated - usually they are 0 for English texts & 'ACGT' data.
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-4-2; else {
					// 'Maximus' uses branched 'if', again.
					if ( \
					( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6 +1]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6 +1]&0xFFFF) ) & ( (1<<16)-1 )] ) == 0 \
					|| ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4 +1]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4 +1]&0xFFFF) ) & ( (1<<16)-1 )] ) == 0 \
					) Gulliver = cbPattern-(2-1)-2-4-2 +1; else {
					// Above line is not optimized (several a SHR are used), we have 5 non-overlapping WORDs, or 3 overlapping WORDs, within 4 overlapping DWORDs so:
// [2x][2a][2b][2c][2d]
// DWORD #4
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>16) =     !SHR to be avoided! <--
// [2x] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) =                        |
//     DWORD #3                                                                       |
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16) =     !SHR to be avoided!   |<--
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) = ------------------------  |
//         DWORD #2                                                                      |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>16) =     !SHR to be avoided!      |<--
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) = ---------------------------  |
//             DWORD #1                                                                     |
// [2d] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]>>16) =                                 |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF) = ------------------------------
//
// So in order to remove 3 SHR instructions the equal extractions are:
// DWORD #4
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) =  !SHR to be avoided! <--
// [2x] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) =                        |
//     DWORD #3                                                                       |
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) =  !SHR to be avoided!   |<--
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) = ------------------------  |
//         DWORD #2                                                                      |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF) =  !SHR to be avoided!      |<--
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) = ---------------------------  |
//             DWORD #1                                                                     |
// [2d] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]>>16) =                                 |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF) = ------------------------------
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-6; else {
// Since the above Decumanus mumbo-jumbo (3 overlapping lookups vs 2 non-overlapping lookups) is not fast enough we go DuoDecumanus or 3x4:
// [2y][2x][2a][2b][2c][2d]
// DWORD #3
//         DWORD #2
//                 DWORD #1
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-8]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-8]&0xFFFF) ) & ( (1<<16)-1 )] ) < 2 ) Gulliver = cbPattern-(2-1)-2-8; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) {
							// Order 4 [
						// Let's try something "outrageous" like comparing with[out] overlap BBs 4bytes long instead of 1 byte back-to-back:
						// Inhere we are using order 4, 'cbPattern - Order + 1' is the number of BBs for text 'cbPattern' bytes long, for example, for cbPattern=11 'fastest fox' and Order=4 we have BBs = 11-4+1=8:
						//0:"fast" if the comparison failed here, 'count' is 1; 'Gulliver' is cbPattern-(4-1)-7
						//1:"aste" if the comparison failed here, 'count' is 2; 'Gulliver' is cbPattern-(4-1)-6
						//2:"stes" if the comparison failed here, 'count' is 3; 'Gulliver' is cbPattern-(4-1)-5
						//3:"test" if the comparison failed here, 'count' is 4; 'Gulliver' is cbPattern-(4-1)-4
						//4:"est " if the comparison failed here, 'count' is 5; 'Gulliver' is cbPattern-(4-1)-3
						//5:"st f" if the comparison failed here, 'count' is 6; 'Gulliver' is cbPattern-(4-1)-2
						//6:"t fo" if the comparison failed here, 'count' is 7; 'Gulliver' is cbPattern-(4-1)-1
						//7:" fox" if the comparison failed here, 'count' is 8; 'Gulliver' is cbPattern-(4-1)
							count = cbPattern-4+1; 
							// Below comparison is UNIdirectional:
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// count = cbPattern-4+1 = 23-4+1 = 20
// boomshakalakaZZZZZZ[ZZZZ] 20
// boomshakalakaZZ[ZZZZ]ZZZZ 20-4
// boomshakala[kaZZ]ZZZZZZZZ 20-8 = 12
// boomsha[kala]kaZZZZZZZZZZ 20-12 = 8
// boo[msha]kalakaZZZZZZZZZZ 20-16 = 4

// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}

							// In order to avoid only-left or only-right WCS the memcmp should be done as left-to-right and right-to-left AT THE SAME TIME.
							// Below comparison is BIdirectional. It pays off when needle is 8+++ long:
//							for (count = cbPattern-4+1; count > 0; count = count-4) {
//								if ( *(uint32_t *)(pbPattern+count-1) != *(uint32_t *)(&pbTarget[i]+(count-1)) ) {break;};
//								if ( *(uint32_t *)(pbPattern+(cbPattern-4+1)-count) != *(uint32_t *)(&pbTarget[i]+(cbPattern-4+1)-count) ) {count = (cbPattern-4+1)-count +(1); break;} // +(1) because two lookups are implemented as one, also no danger of 'count' being 0 because of the fast check outwith the 'while': if ( *(uint32_t *)&pbTarget[i] == ulHashPattern)
//							}
//							if ( count <= 0 ) return(pbTarget+i);
								// Checking the order 2 pairs in mismatched DWORD, all the 3:
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1]] == 0 ) Gulliver = count; // 1 or bigger, as it should
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1]] == 0 ) Gulliver = count+1; // 1 or bigger, as it should
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1+1]] == 0 ) Gulliver = count+1+1; // 1 or bigger, as it should
							//	if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1]] + bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1]] + bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1+1]] < 3 ) Gulliver = count; // 1 or bigger, as it should, THE MIN(count,count+1,count+1+1)
								// Above compound 'if' guarantees not that Gulliver > 1, an example:
								// Needle:    fastest tax
								// Window: ...fastast tax...
								// After matching ' tax' vs ' tax' and 'fast' vs 'fast' the mismathced DWORD is 'test' vs 'tast':
								// 'tast' when factorized down to order 2 yields: 'ta','as','st' - all the three when summed give 1+1+1=3 i.e. Gulliver remains 1.
								// Roughly speaking, this attempt maybe has its place in worst-case scenarios but not in English text and even not in ACGT data, that's why I commented it in original 'Shockeroo'.
								//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+count-1]>>16)+(*(uint32_t *)&pbTarget[i+count-1]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = count; // 1 or bigger, as it should
								// Above line is replaced by next one with better hashing:
//								if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+count-1]>>(16-1))+(*(uint32_t *)&pbTarget[i+count-1]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = count; // 1 or bigger, as it should
							// Order 4 ]
						}
					}
				} else Gulliver = cbPattern-(2-1)-2; // -2 because we check the 4 rightmost bytes not 2.
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);

		} // if ( cbPattern<=NeedleThreshold2vs4swampLITE )
		} // if ( cbPattern<=NeedleThreshold2vs4swampLITE )
	} //if ( cbPattern<4 )
}

// Fixed version from 2012-Feb-27.
// Caution: For better speed the case 'if (cbPattern==1)' was removed, so Pattern must be longer than 1 char.
char * Railgun_Doublet (char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern)
{
	char * pbTargetMax = pbTarget + cbTarget;
	register uint32_t ulHashPattern;
	uint32_t ulHashTarget, count, countSTATIC;

	if (cbPattern > cbTarget) return(NULL);

	countSTATIC = cbPattern-2;

	pbTarget = pbTarget+cbPattern;
	ulHashPattern = (*(uint16_t *)(pbPattern));

	for ( ;; ) {
		if ( ulHashPattern == (*(uint16_t *)(pbTarget-cbPattern)) ) {
			count = countSTATIC;
			while ( count && *(char *)(pbPattern+2+(countSTATIC-count)) == *(char *)(pbTarget-cbPattern+2+(countSTATIC-count)) ) {
				count--;
			}
			if ( count == 0 ) return((pbTarget-cbPattern));
		}
		pbTarget++;
		if (pbTarget > pbTargetMax) return(NULL);
	}
}

// Last change: 2015-Oct-11
// If you want to help me to improve it, email me at: sanmayce@sanmayce.com
// Enfun!

// To see next images select in Notepad font: Lucida Console and size: 4:

/*
YyVyVyYyVyVyVyVyVyVyVyVyVyyyVyVyyyYVVyyyVyyVYVYyYylyYyYVlVYylylVYVlyYyYyVyYylVlVYVYVYylVYylylVYyYyYyYVYyYVYylVlylVYVlVYVYVYVYVlylylyYVYylVYVYylVVyYVlyYVYyYVYVlVYVYVYVlyYVYVYVVVYVYVYVYylyYVlVlylylyYyYVYylVYVYyYyYylyVyYyYyYVlVlyYylVlylVlylyYVYylylyYylVYyYVlylVlylVYVYVlylVlyYVlylylVlVYVlylVlVlVYVlVYVlVlVYVYVlylVYyYVYVlVYVlVYylVlylyYVVyYyVyVyVyVyVyVyVyVyyyVyyyVyVyVyVyYyVyVyYyVyVyVyVyYyYyVyVyVyVyVyVyYyVyVyVyYryyVyVyyyVyyryryryryrVryLyLyryyyryryyyyyrVryLyyyLyLVrVyyyyLyryyVLVLyLyryrVyrrVLyyVyyArryrLAyAyLyLVryryryAyryryrLAyryrr
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvvvvvvvvvvvlvlvlvlvvvlvlvvvvvvvvvlvvvlvlvlvlvlvlvlvlvlvlvlvlvlvlvlvlvlvlvlvVlYvlvlvlvVvYvlvvlVvlllvYlYlYvYvYvlvV
YyYyYVVyVyVVYVlVYyVVlVVVlyVVVyYVlylVlVYVYVYVlVYVYVYVlVYVYylVlVYVlVYVYVlVYVlVlVYVlylVlVlVlVlVYVlVlVvVlVYVlVYVYVYVlVlVlVYVvVlVvVYVlYlVvVlylYlVYVlYlVlVlYlVlVlYlVlVlYlYlYlYlYlYlVlYlVlYvVlYlYlVvYlYlVlVlYlYlVlVlVlYlVlVlVlVlVlVvYlVlVlVlYlVlYlYlVlYlVlYlYvVlVlVYVYylVlVlVlVYVlYYVlVlVYVlVlYlVYVYylVlVlVlVYVYylVlVYVYVYVYVYVYVlVlVYVlVYylVYVlyYVYylVlyYylVlyYVVylVYylylyYVlyVVYyVVYyYyVVlyYVlyYVYVYyYyVVYyVyYyVyVyYyVyVyYyVVVyVyVyVyVyVyYyyyyyyyyyVyVyVyyLVyVLVyVyVyVyyLyLyLVLyyyyyyVyVLyyVLyryryryrVryyVLVyyLyryrVryLyyyyyLyryrVryryLyyVryryryyr
yYVlyYylyVVYVlYlVlylVlyYVlVYVlylVlVlVYylVYVlYYVlVlYlVlVYVlVlVlVlVlYlVlVvYlVvVlYlYlVlVlYlVlYlVlYlVlllYlVYVYVYVlVlYlVlYlVlYlllYvVlYlYvYlYlVlYvVvVvVvllVvYlVlVlYvYYVvYvYvVvYvYvVlYlVlYvVYlvYlYlVvYlVllvYvYlVlVvYvYlllVllvYlVvllYlVvYlVvlvlvVlVlYllvVvYvYlllVlVlVlVlVYVlYlYlYllvYvVlVvVlVvYvYvVlVlVlVlVlYlVlVlVlYlVvVlVlVlVlYvYlVlYlVvYlYlVYVYVlVlYlVlVlVYVlVYVlVYVlyYVYVlVlVlVlVYyVyYVYVlVlVVylYlVlVYVlVVylVYyYyYyYyVylVlyYyYylyYVYyYyVyVyYVVyVyVyVyVyVyyyVyVyVyVLVyVLVyVyyyyLVyVyyLVyyLVyVyVyyyyLyyyyyLVLVyyyyrVLyrVyVLyrVyyryLyLyLyLyryryryrVr
YVVyYVlVlVVVYVYVYylyYyYVlVlYlVlYlVlVlVlYlVlVlYlYvVlyvVlVvYlVlVlVlVlVlYlVlVvVlVvYvYlVlVlYlVlYvYvVlVlYvVvYvlvlvlvVvlvYlYvYvlvYlVvlvVvlvYlllYvVlYlYvlvYlYvVvlvYlVvYvYvVvYlllYvYvYvYvlvYvlllvYvYlYlllVvYvYvYllvYlYllvVlYlYllvllVvYlYllvVllvYvVvlvYlVvYllvYvYvYlYlYvYlllYlYlYlVvVlYlVlYlYlYlVlYlVvYlYvYvVlYvVlVlVlYlVlYlVlYvYvVlYvYlVlVvVlVlVlVlVlVYYlVlVlVlVlVlVlVlYYVlVlVYVlVlVYVlVlVVVlVlylyYVlVYVlVlVYVlVlVVVlVlVYVVVYVYVYylVVVYyYylVVyVVYyYylVYyVyYyyyVyVyYyVyYyVyYyVyVyVyVyVyVyVyVyVyVyVyVyyyVyVyVyVyVyyyVyVyVyyyVyVyVyyyyyVyyyyLyLyyyryryyr
VYyYyYyYylVlVlVlYlVlVlVlVlVlVlYlVlVlYvVvVlVvYlVlYlVvYlVvVlVlVlYvVlVlYvVlYlYlYlYlVlVlVlYlYvVlYlYlYvYvVlYlllVvllYlVvYvlllvYllllvlvllYllvYvYvlvllYvlvlvVvYvYvYvlvlvllVvYvVvVvlvVvYlYlVvYlVlYvYvVvYlYvlvlvYvYvVlYllvllYvYvlvYvlvYlYvYvllYvlvVYVvYvYlYvlvlvYlYlYvlvlvVlYvYvYvYlYlVlVlVvYvYvVlYlVlVlYlVlVlYvYvVlVvYlYlVvVlYlYvYlYvVvYvYvVlYlVvVlYlVlVlVlVlVlVlVlVlVlVlYlVlVlylVlVlVlVlVYVYVYVlVYVYVlVlVlVlVYVYylVlyVVYVYVlVYVlylVYyVVVVVVYVYVYVVyYVVyyyVyYVlyYyYyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyyyVyyyyyVLVyVLVyVyyyVyyyyyyyyyVyVLVyyLyrVLyLVr
YVYYlYlVlVYyVyVylVYVlVlVlVlVYVlVlYlVlVlVlVlVvYlVlllYlYlYlVlYlVlYlVlVlYlYlVlVlYvVvVlYvYvVlVlVlVvYlYvVlYvVvYlYlYlllYvYvllYllvYvYvYvYvlllvlvYvlvlvYlYvYvllVlYvlvVlYvYvYvlvllYlllVlYvlllvYvYvYvYlYvYvYvVvlllvYvYvlvlvVvlvVvYvVvVlYvYvYvlvllYlYvYvYvVvYlYlYlYlVvVllvlvYvllVlYvVlVlYlYlVlVlYvYvllYvllVlVlYlVvYlVlYlYlYlYlVYVlVlVvYlYlVlYlYlYlVlVlVvVlYvVlVlVlVvVlVlVlVlVvYYVYVlVlYYVlVYyYylVlyYVlVlylVlVYVYVYVYylyYyYVlylVlylyVVYVYVYyVyYVYVyyVVYVYyVyVyVyVyYyVyVyVyVyVyVyYyYyVyVyVyVyyyyyVyVyVyVyVLVLyyVyVyVyyyyyVyVLVyyLVLyLyLyyyyVLyyyyyryryyyyr
VlVlVlVlVlVlVYYlVVyVVVylYlVlVlYlVlVlYlVvVvVlVlVlVlVlVllvYvVlVvVlVlYvYlYvYlYlYvVllvVlVlVlYvVvYvllVlYlVvYvlvlvVvllVvllYvVvYvYvlvYvYlllYllvvvvvlvYlVlVlYlYlYvYvlvYvYlYvYlYvYvYvYvYlYvVvlvYlllVvYllvlvVvVvYvVlYvYvYvYvYvYlYlYvlvlllllvYvYvlvYlYllvVlYvlvYvlvlvVvYvYvVlYvVlYlVvYlVlVlVvVlVlVvVlVlVlVlVvYlYlVvVlVvYlYvVvYvYlVvVvYlVlYvYvVvYvYlYlVlVlVlYlVvVvVlVlVlVlVYVlVlVlyYVYVlylVYYlyYyVVYVlYYVlyYVlVlyYVlVlVYVYyYVYVYyYVYVYVVyYyVVlVVyYVlyVVVyVVVyYyVyVyVyYyVyYyVyVyVyYyVyVyVyVyVyVyVyVyVyVyVyVLVyyLVLVyVyyrVLVLVyyyyyyyVyVyVyyLVyVyyyyyyLyLYr
lVlVlVlVlVlYlVlYlVlVlYlVlVlVlYlYlYlVlYlVlVlVlVlVlYvYlYlVvYlVlVvYlYlYvlvlvlvVlVlVlVvVlYvYvYvYvYvlvllVvYvYvYvlllvYvYlYvlvYvYvVvYlVvYvvvlvlvTmr;lvYvYvYvVvYlYllvVvlllvYlYvYlYlYvYvYlYvlvllYllvYvlvYvYvYvlvllVlVvlvVvYlllYlYvYllvYlllYvYvVvVlYlllYvlllvVlYlVlYvllYvlvVvYlYlVvlvVvlvYvlvYlllYvYlllYvYvVlVvVvVlVlVlYvllYlVlYlVvVlYlVvVlVlVvYlYlYlYlYlYlVvVlVlVlVlYlVlYYVlVlVlVvYlVlVlVlylylVvVlVlyYyYyYVYyYVYVYVYyYVYyYyYVYVlVlylVYyVVYVlVYVVVlVVyVylyYylyVVVyVyVyVyVyVyVyVyVyVyVyyLVyVyVyVyVyVyVyyyVyVyVyYyVyyyVyVyVyVyyyVLVLyyVyyyyyVyyyVLyrVryyy
YlVYVlVlVlVlVlVlVlVlYlVvVYYlVlVlVvVlVlYlYlVlVvVvVlYvYlVlYlYlVlYlVlYvYlVvVlVlYlYvYlVlYvYvllYvYvYvlvlvYvYvYvVvYvllYvlvYvYvYvVlYvlvl;vvvv;NYvvevvYlVvYvYlVllvYvVvllYvYvVlYvlvlllvVvlvYvYlYllvVvlvYvYvYlllYvllYvVvVvYvYlllYlllllYvYvlvYvYvYvlvVvYvYvYllllllvlvllllYlYvYlVlVvYvVvlvYvYvYlYlYvYlVvYvVlYvVlYlVvYlVlYllvVlYlVlVlYvVlVlYlYvVlVvVvVvVvVlVlYlVlYlVlVlVvYlYlVlYlYlVvVlVYVlVlYVVYVvVlVlVYVlVlVYVlVYVYylVlVYVYylyYylyYylyYVVyYVlyYVVVYyVyVyYyVyYyYVYyYyVyVyVyYyVyVyYyVyVyVyVyVyVyVyVyVyVyyyVyyyVyyLVyVyVyyyyLVyVyVyyyVLVLyLVyVyVyyyyLyLVrVA
lVlVlVlVlVlVlVlVlVvVlVlYlVlVlYlVlVlVlVvVlVlVlVvYlVlYlVvVvVlVvVvVlYlVvYYVlYlYlVvVlYlllYlYlYvVvYvlvVvlvYvYvYvYllvVlYvVvYvYlYvlYYvv;:vlv;l6,;:8j;vVlVvlvlvlvllllYlYvVlVvllVvlvlllvlvVvlvVlVvlvYllllvlvVvYlVvYvYvYllvlvlvlvYvllYvYvlvlvYvVvlvVvllllllYvlvlvlvVlllYvllYvYvYvYvYvYvYvYllvVlYvVvVlYvlvVlYlYvVlVvVlVlVvVlVlYlVvVlVlYlVlYlVlVlVvYvVlYvVlYlYvYvVlVlVlVlVlVlVlVlVlVlVlVlVlVlVYVYylVYVlVVVlyYyYVlVlVlyYyYylVlyVVlVlylVlylVlVYVYVYVlyYVVyVyYVlyVVVyVyVyVyYyVVVyVyVyVyYyVyVyVyVyVyVyYyVyVyVyyyVyVyVyyLVyVyyyVyVyVLVyVyyyVyyyyyVLVyyryryyyyr
YlVlVYYlVvVlVlVlYlYlVlYlYlVlYlVlVlYlVYYlVlVlVlVvVlVlYvVvllVvVvVlYvVvYvVvVlVlYvYlYlllYvYvYvYvYllvYvYvYvYlYvVvlvYvYlYlYvYlYlYlv;;;lO8BevLy v;B8;vvVllvYllvYlYvYvlvllYvlvlvYvlvYvlvlllvVlllYvYlllYvlvYvYlYvllVlYvVvYvYvYlVlllYvlvYlVvYllvllYvVvlvYlVvYlYvlvYvYlYllllvYvVvYvYllvlvVvVvYvYvYlYlVvVvYlVlVlVlYlYlYlVlYvVlYvVvYvYlYlVlVvVlYlYlVlYlYvVlVlVlYlVlVlVvVlYvVlVlVYYlVlVlVlVlVlVlylylVlVlYlVlVlVlVYVlVYVlYvVvVlVYVYVlylyVyYVVVlyYyYyYylVlVlyYylyYyVVYyVyVyVVVVlyVyVyVyVyVyVyVyVyYyVyVyYyVyVyVyyyyyVyVyyyVyVyyyyyVLyyVyyLyyVLyLyLVyyyyLyryLYr
lVlVYYlVlVlVlYlVYYlVlYvylYYVlYlVvYlVlVlVlVlYlYvYvYlYlVlVlVlVvllYvVvVlVlYlVlVvYvYvVvlvYvYvYlYvlvllYvYvlllvlvlvYvYlYllvYllvYvv;;veB8B8B8BV:v;BBv;vlVvVlYvYlYvVvVvlvlvlvYvVvYvlvYvYlYvYvYvVlllYlVlVlYvYvYvYvYlllYlYlYvlvYvYllvYvlvYlYlYvYlYvYvYvYvVvYvllllYllvYlllYvYvYlllYvYvYvVlYlYvVvYvllYvlvllYvlvYlYlVlVvVvVvYlVlYvYvYvVlYvVYYlVvVlYlVYVlVlVlVvYlVvVlVlVlYlYlVlYlYlVlYlVlVlYlYlVlVlVlVlVlVlVlVlVlyYVlVlVlVlVlVYVlyYylVYVYVYVYVlylyVylVYVVylVYyVyYVYyYyVyVyVylylyVyVyVyVyVyVyVyYyVyYyVyVyVyVyVyVyVyVyVyVyyLyyyLyLVyVyVLVyVyVyVyVLVLVyVyVyyyy
VYVlYvYYVYVlVlVlVlYlVvllYlVYVlVlYlVvYlYvYlVlVlVlVlYlVvYlYvYlVvVvVvVvVlVlVvYlVllvYvYvVvlvVvVvYlYlYvlvYvllYlYlYlVvYvYvYvVllv;,ve8B8e8B8B8cv;;98w;vllllYvVlVvlvVlYvYvlvYlllYvYvlvYvllYlllVlYlVvlvYvlvlvYlVllvVvYlVvlllvYvlvYlYvllVvllYlYvYvlvVvlvYllvYvYlVvYvlvVlYlVlYvVvlvllYlYvlvllYvYllllvYlYvlvYvlvYvVvVlVlVvYvYlYvVvVvYlYvYlYvYlYlYlVvVlYYVvVlYvVlVlYlVlVlYlVlYvYvVlVlVlYYVlVlYlVlVlVlVlVlVlVlVlYYyYVlVYVYVVVYVYyYylyVVVVVylVlVVVVyVVYVlVlyYyYyYyVVVyVyYylyVyVVVyYyYyVyVyVyYyVyVyVyVyVyVyVyVyVyVyyyVyVyVyVyVyVyVyVyyyyLyyyyyyyLVyyyVLVyVLYL
lVlVlVlVlVlYlVlYYVlVlVvVvVYVlVlYlVvYlVlVlVlYlVlYlYvYlVvYlYvVvllVvVvVvYllvYvVlVvVlllllllYvYvYlYvllYllvVllvYlYvYvVvYvVlYvv;:;CB86Ge8B8Btv;Vr;yG6vvvVvYvYvYlYlYlYvYvVlYllvlvVlYllvlvVlYvlvYvYvlvYvlvlvVvllllVvlvYvVvlvlvYllvlvlvVlYvlvYlVvlvllYvlvVvllVvVvVvYvYvVllvlvYvVvVvYlYlYvYvVvYvlvYvVvYvlvVvYlYllvVlYlYlVlVlVvVlYlVlYlVvYlYvVlVlYvVvYlVvVvVlYlVlYlVlVvVlVlVlVlYlYlYlYlYlVlVYVlVlVYVlYlVlVYVlVlyVylYlVYVYyYVYVYVlyYylyYVYVlyVyVyVyYyVVYyYyVVlylVVyYyVyVVYyYVYyVyVyYyVyVyVyVyYyVyVyVyVyVyyyVyVyVyVyVyVyyyyyVyVyyyVyyyyyVyVyyyVyVyVyVLVLVyy
VlVvVYVlVlVlVlVlVllYVlVYYlVlVlVlVlYlVlYlVlVlVvVvVlVlYvYlYlVlYvYlVlYlVvYlllYvVlYvYvYvYvllVvllYlYvlvYlYlYvYvYvVvYvYvYlvv;,vp8B8e88888@V;yG8B@;;mAvvYlvYlYvYvYvYvYvVllvYvVllvlvlvllYllvYvYvYlllVlllYllvlvYvYlllllYvllllYvlllvlvlvllYvlvlvllYvlvYlYvYvlvlvYlVvYvllYvVvYvlllvlvYlVvVvVvlvYvVvVvYvYvVvVlVvYlYlVlVlYlYlVvVlVvYlYvVlVvlvYlYlYlVvlvVYYlYlYlVlVlVvVlVlVlVlYlYlVlYlYYVlVlVlYlVlVlVlVlYlYlVlYlVYVYYlVYyYyVVYVlVYVYVlVlyYVYVVylVYVlVlVVVVyVyVyYyYyVVVyVyYyYVYyVyVyVyVyVyVyVyVyYyYyVyVyVyVyVyVyVyVyVyVyVyVyyyVyVyyyyLVyVyVLyyyyVyyyyLyyyLVr
vVlVlYvVvYlYvYlYYVlVlYlYvYvYvVlYlYvVlYlYvVYVvYllvVlYlYlVvVvYvYvYvVvYvVvllYvVvYvYllvVvYlVvllYvYllvYvYvYvYvYvYvYlYlYlv;;;kB8B8B8B8B6Y;v#B8B888qrAvvvlYvlvllYvlvYlYvlvllVlYlVlVvVvYvYvllYlYvYllvllYlYlYlYvVlVvYvYvYlYvYlVlVvVvlvlvlvYvVvlvlvYvlvYllvVvYlYvlvlvlvlllvYvYvYvVlYlYllvlvYvYllvlllvYvllllYvlvYlllllVYVvYvllYlYlYvYvVlVYVlYlVlVlYvYlVvYlYlYlVlYlVYVlYlVlVlYlYlYlVvYlYlYlVlVlVlYlVlVlVlYlVlYlVlYlYlylVlVlVYylVYVYylVlVlyYylVlyYVlVlVVyYyYyYyVyVVVyYyYyYVVyYyVylyYVVyVyYyVyVyVyVyVyVyYyVyVyyyVyVyyyVyVyVyVyyyVLyLyyVyVyVyVyVyVyVyVLVyVyy
VlVlYlVlYlYlYlVvVYYlVlVlYlYlVlVlVlVlYllvVvVvYlVlVvVlYlVlYlYvlllvVvYlYlYvYvllYvYvVvYvYvlvlvlllvYvYlYvVllvYvlvlvYvYvv:,l8B8B888B8eM;;p8B8B8B888H;vvvvvVvlvYvlvvlYlllYvlvYlllYlVllvVlYvYvllVvYlllYvVllvlvYvYvVvYllvYvYvYvVvlvYvVlllYvlvYlYvYvYlVlYvlvYvYvlllvYvVlVvlvlvlvlvllYvlvllYlllYvYlllYlYvYlllYllvlvYvYllvYlVvVvYvVlYvVlYvYlVlVvYvVlVvYvYlYlVlVvVlYlVlVlYlYlVvVvVlVlVvVlVvYlVlVYYlVYYlVlYlVlVlYlYlVlVYylylylyYVYyYVYVYylVYVYyYVlylVYVYyYyYVVyYyYylyYVYylVYyVVYyYyVyVyVyVyYyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVLVyyLVyyyVyVyyLVLyyVr
lVlVlVlYlYlYvYlVlVvYvYvYlVlYvYvVlYlVlVvYvVllvVlVlYlVlVvVlYvYvYvYlYvYlVllvYvVlYlVvlvYvVvlllvVllvlvllYvYvYvVlYlVvlv;:v@8B8B8B8B8t;;Ae8B888B86H;;rkvvvlyhX4ArVYvllYvYvllVlVvYvYvVvYlVvlvVvVvlvYvlvYvlvlvYvYvYlllYvllVlYvlvVvYllvYlYllvllYlYvVlVvYvllVvYlllYvYvYlVvYvYvlvYvVvYvYvYlVvVvllYvlllvYvYlllYvYvYvYlYvYvYvVlVlYvVlVlVvVvYlYlYlYlVvYlVvVlYlVlYvYlYlVlVlYlVlVlVlVlVlVlVlYlVlVlVlVlVlVlVlVlVlVlVlVYVlVYVYVYVlVlVlVYVlVYyYVlVlylVYylVYyYVVVYyVVlylyYVlyVyYVVVYyVyVyYVYyVVVyVyVyYyVyVyVyVyVyVyyyyyVyVyVyVyVyVyVyVyVyyyVyVyVyVyyyVyyyVyVyyLyyy
YlVlVlYvVlVlVlVlVlYvlvYlYlYlVlYlVvVlYlYYYlYlVvYvYlVlVlVlYvYlYvllllYlllVlYlYYllYvYvVvYvYvVvYllvYvYvYvYvYllvlvVvv;:;C88B8B8B8Bj;;ABB8B8B8BBw;;wG888jtkM;:;L4kXrvYlYvYllvYlVvllYvYlllYvVvYlVvVvYlYvVlYvYvYvlvYvlvlvYlYvYvlvVvlvYlYvlllvYvYvllVllvYllvYvVvYlYvlvlvYvlvllYvYlYvYvVvYlVlVlYlYvYlYvYvYvllYvYvYlVlYlVlYllvYlVvYlYlYlYvYlYlYvVlVlYvYlYlVlVlYvllVlYvYvVlYlVlYlVlVlVlVlVlVlVlYlVlVlVlVlVlVlVlYYVlVlVVVlyYylVlylVYylyYVlVlVYVlVlVYylylVYVVVYyVyYylyYVVVlyVyVyVyYyVyYVVyYVVyVyVyYyVyVyVyVyVyVyVyyyVyVyyyVyVyVyVyyyVyyyVyVyVyyLyyVyyyVyyLVL
YVlYlYvVvVlVlVllvYlYlVlYlYvVlYvVlYvllYvlvlvYvlvVvYllvVvllYlllVlVlYvVvYvYllvllYvlvlvYvYlYvYvYlllYvYvVvVvVlYvYvv,;48B8B888B8Dl;yO8B8B8B8Bt;vr9B8B8B8v        ;rwYYvVvYlYllvYvVvYvlllllvYvVvVvllYvYlYvVvllYvYvVvllVvYlYllvVllvlvVvlvVvlvYvllYvYlVlYvlvVvllVllvYlVvYvYlYvllYvlvYvVvYvYvYlllYvYvYvlvlvlvYvYvYvYllvYvlvYlVlYvYvVvVlVvVlYvYvYvYlVlVvVlYlVlVvVvYvYlVlYlVlVlYlYlVvYYylVlVlVlVvYlVlVlVYVlVYVlYlVlVlYvVlVlVlVYylVlYlylVYVYylylVlVlyYVYyYVlVlVYyYyYyYyVVVyYVVyVyVyYVYyVyVyYyYyVyYyVyVyYyVyVyVyVyVyVyYyVyVyVyVyVyVyyyVyVyVyVyVyyLyLyyVLyyy
VlVvYlVlYlYlYYVlYvYvVlYlVvYlVvVvVlYvVlYvYllvllYvYlYvlllvlvlvYlYlYlYvVvllllYvlvYvYllvllYvYvlvlllvlvYvYvYvVvvv;:lB8B8B8B8Ber;;0B8B8B8B8DA;AO8B8B88v            yhlYllvVvlvllVvlvllllYlYllvYvYvYvllYvYvYvllllYvlvYvlvlvYvVvllllVvlvlvVvlvYlVvlvYlVvYlYvlvYvYvYvYvlvlvVlYlYvllYvYvlvlvYvlvYvYlllVlYllvYvlvYvYlYlVlYlVvVlYvYvllVlYlYlYlllYlYvllYlYlVlVvYlVlVvVvYlVlYlVlVlVlYYYvVYYlVYVlVlYlVlVlVlVlVlVlVYVlVlVlVlVlVYVVyYylYYVlylVlylVYVlVYVYyVyYylyYVYyYyYVYyYyYVYyYyyyYyYVlyYyVyVyVyVyVyYVYyVyVyVyVyYyVyYyYyVyVyVyVyVyVyyyVyyyyyVyVyVyVyVyyyyyYr
lVlYlYvVvVlVlYvVvYlVlVlYlYvYlllYvYvYvVvYllvVvYvYvllllYvlllvlvYlVlYlYvlvlvVvllYlYvllVlllYvYlYlYlYvYlVvYlYvv::vB8888B8B8Bk;;H8B8B8B8BekvvbB8g8B8p           .   VUYYlYvYlVlYlYvYllvYvlvYvVlllVvYvYvYvlvVvVvYlYvYvlvYllvlllvlvYvlvllYllvVlVlVvlvVlYvVlYvYvYvYlYlVlYvVllvlvYvllYvYvVllvYvVvYvlvVvYvlvlvVvlvYvVlYllvYlYvllYvYlYlYlVlVlVvVlVvYvllYvYlYlVvYlVlYlYlYlYlVlYlVlYlVvVlVlVlYlYlVvVlVlVlVYYlVYVlVYYYVlVlVlYlVlVYVlVlVlVlVlVlyYVlylVlyYyYylVYyYVlVYyYylVVVVyVyYyYyVVVVYVVyYylylyVVYyVyYyYyVyVyVyVyYyVyVyVyVyVyVyyyVyVyyyVyVyVyVyVyyyVyVyVyy
VlVvYlYlVvVlYlVYVlYlVlYlYlVlVvVlYlVvVllvlvYvYvlvYlVvYvYvlvYvllVlYllvlvYvlvVvVvlvVvYvYvYvlvYvllYvllYvlvYv;,vc8B8B888B8C;,AB8B8B8B8Bc;;k88B88B8;        NB88mL,  YkYlvYvlvlvYvVvlvYlYvYlYvYlYlVlYlYvYlYlYllllvllYvVvYllvYvlvYvYvYvlvYvllYlYlYvllllYvlvYvllllVvYvVllllvlllvYlYvYvYlYlVvYvYvlvlllvYvlvYvVvlvYlYvYvYlYvlvlvVlVvYvVlYlVvYlYlYlYlVlVlVvVlVlYlVvVvYlYlVlVlVvVlVYYlVlVlVlVlVlVlVlVlVlVlYlVYVlVYYlVlVlVlVlYlVlVlYYVVVlVYylylVlVlVYVYVYyYVYVYVlylVVyYyVyYVlyYyYylVYyYVVyVyYVlylVVVyyVyVyVyVyVyYyVyVyYyVyYyYyyyVyVyVyVyVyVyVyyyVyVLVyVLVL
lYvYlVlYvYvVlVvlvVlVlYlYvYlVvlvlvVvYvllYvVlVvlvYlYlllVlllYvYlYvYlVllllvYvYvlllvYvVvVvYvYvYvYvVlYvllVvv;;;kB8B8B8B8BGv;ABB8B8B8B8jY;Ug89eB8Br       :O8B88OyLV.  YUVlllvYvYvlvYvYvlvVvVlYlVlYlYllvYvlvlvlvYllvlvYvlvlvllVvVvYvlvYvYvlvYvYvYlYvllYvYvYvVvYvYvYllvYvYvVvlvlvYvlvYlYvlllvYlYvlvVlVlYllvlvllVvYlVlVvYlYlVvYlYlVlYlYvVlYlYvVvVlVlYlYlYlVlVvYlVlVvVlVvVlVlVlVlYlYvVlVlYlVlVlVlYvVlYlVlVlVlYlVlVlVlVlYlVYVlVlYlVlVlyYyYVvVlVYVYVlVYVVVlylVYVYVlVYyYVYVYylVlyYVlyYyYVlyYVVyVVYyYylVlyYyYylyYyVyVyVyYyVyYyVyyyYyVyVyVyVyyyVLVyVyyLVyyyy
VlVvVlYlVlVlVvVvVlYlYlVlVvYvYvlvYvlvllVvYlVvVvYvVlVvYvllYvVvYvYvVvlvYvlvVvYvYvYlVllvYvYvVlYvlvVlYvVvv:;YBB8B8B8B8BU,vC8B8B8B8B9V;vDB86888B:       8B8B8BZccvy,   VmyYlvvlvlvYvlvlllvYllllvYvVllvlvYvYlYvllYlVvllYlYvYvVvlvlvVvlvYvlvYlVvYvYlYvYvYvVvlllvYvYlVlYllvYlYvYllvYvVvVllvVvVlYlYllvYvYlVlVlVlllVvVlVvVlVlYlYlYvVlVlYlVlVlYlVlYvlvYvVlVlYvVvVvVlYvVvYlVlYlVlVlYlVlVlVvYlVlVlVlVlVvYvVvVlVlVlYlVvVlYlVlVlVlVlVlVlVlVlVlVlYlVVVlyYylyYVYVYVlVlVYyYVlVlVVyYyVyYylylyVVYyYylyYyVVYyVVlyVyVyVyVyVVlyVyVyVyVyVyVyVyVyyyVyYyVyVyVyyyVyVyVyYr
lYvVlYlVlYlYlVlYvlvVvYllvVvYlVvYllvYlYlllYlYvYlYllvVvllVlYvYvlvYlYvYlVvYvlvVvlvYvYvYvYllvYvYvYvlll;;;v98B8e8B8B8c;;jB8B8B8B8Br;v#8BGe8B8B       08B8B86p98B8cA:   ,jUTyVvVvVvYlYllvlvlvYlYvlvYlYvYllvllVvlvYlYvYvYllvlvYvYvVvlvlvYvlllvllVvYvYvYvlllvllYvYvYvllYlYvlvYvYvllVvlllvYlYlYvYvYvYvllVlYlVlYlYlYlYvYvVlYvYlYlVvYlVlVvYlYlYlYlYlVlVvYlVvVvVlYvYvVvVlVlVlYvVlVvYlYlYlVlVlYlYvVvYlVlVlVvVlVlVlVlVYVlVlVlVlVlYlVlYlVYVlVlYYVlyYVYyYVVVlyYVlVYyYylVlVYyYyYVlVYylylyYyYyVVYVYVlylyVyYVYVlyVyYyYVYVVyVyVyVyVyVyYyVyVVVyVyVyVyVyVyVyVyVLyyy
VlYlYlYvYvVlVlYvlvYlllYvVlVlYvYlYvVvYvllYlVvllYvVllvYvYvlvYlYvYvVvlvYlVlYvYlYvYvYvYlYvYvVvYvYlYvv;;vDB8B888B8B9v;X8B888B8B8Hv;kB8BBB8B8B      ,B8B8B89ee8B8B888j:   ;UcUUryYYvlvYvVlYlYlYvlllvlvlvVvVvVvYlllYlVvYvYvlvYlYvYvYvVvVllvlllvllYvVvYvYvYllvYlYvllllVvlvlvllYlYvlvlvYllllvlvYlYvYvlvYllvYvllllYlVvVlYvYvYlVlVlVlVlVvYvVlVlYvYlVvVvYvYvVlYlVlYlVlVvYvVlYlVlVlVlVvYvVlYvVvVlVlYlVlVvVlVlYlVlVYVlVlVlVlVlVlVlVlVlVlVlYlVYVYVVylVYylYlVlVYylVlylVYylVYyYyYyYyVyYVYyYVYVlyYyYyYyYyVylVYVYVYVlyVyYyVylVVyVyYyYyVyVyYyVyVyVyVyVyVyVyyyVLYy
lYlVlVlYlVlVvYlYllvYvlvVlYlVlllYvYvVlllVvVvlvYllvYlYvYvVvYvYvYvYvllVvlvlvYlYvYlYvllYlYvVvYlYvlvv,;t8B8B8B8B8eY,VB8B8B8B8B0v;A8BBBBB8B8B      ;B8B8B8B8B888B88; .        ,lMylVvYvYvYvllllllYllllvVvllllYlYllvYllvllYlYvYllllvYvlvYvYlYvlvlvlvYlYvVlllYvlvlvlvlvVvVvlvVvllVlVlVvYlYlllVlYvVvlvllYlVlYlYvYlYlYvYvYlVlYlVvVvllYlVlYvVlVlYvYvYvVlYlVlYlVlYlYlVlYvVlVlVlYlVvYvYlVlVlVvVvYlVlVlYYVYVlVlVYVlYYVlVlVlVlVlVlVlVvylyYVlyVYlVlVvVlVlylyYylVYVlVYVYVlVlyYVYyYVYylylyYVYVlVYVYyYylVVyVyVylyYVVyYyVyYyYyVyYyVyVyVyVyVyVyVyVyVyyyVyVyVyyyVVy
YvVlVlVlYlYlYvYllvYvVvllYlYvYvYvYlllVvVllvlvVlYvYvYllvlvYvYvlvllVlYvYllvYvYvYvllYvVvlvVlVvlvl;;;AB8B8B888B8k;vZB8B8B8B8Or;AO888B8B8BD2      VB8B8B8B8B6;;,                vTYvllYvYvYlVvYvllllYlYvYvYvYvYvYvlvlvVllvYlVlllVvYvVvVvllYvYvllYlYvlvYvlvYlYlYlYlYlVvYlYvYvVlYvlvYvYlYllvYvVvllYvllYlYvVlYlYlVvYlVlVlYvYlYlYvVvVlYlYlYvYlYlVlVlYvVlVvYvVlVlYlVvVlYvVlVlYvVlVlYvVlYlVvVlYlVlVlVYVlVlVlVlVYVYYlylVVyYVlVlYlVlyYVYVYylVYVlVYVlVYVYyYVYYYVYVlyYVYylyYyYylylVYVYylVYVYVYyYVlyVVlVYVVyYVVyVylyYVYVYyYVVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyVyYL
lYvVvVvVvYvVvlvlllllvlvVlVlVlYvVvYvYvllVvYvVlVvVvVllvVvVvYvYvVvlvlvllVvYvYvlvYvllYlVvYvYlYvv:;lBB8B8B8B8Bp;;k8B888B8B8A;vZ88B8B8B8UvBO     .B8B8B88Z                       MylvlllvlvlllvYlllYvYlllVlYlVlYvlvlvYllvVvYlVlVlllYvVvlvlllvYvYvlvlvlvYvYvYlYvlvlllvYvYvllVvllllYvllYlllVvVvYlllVvYlVlVlYvVlVvVlVvYvVvYlYvVvVlllYvVvVvYlYlYlYlYvVlYlVlYvVlYvYvVlVlVlVlVYVlVlVlYlVvVlVlVlVlVYVlVlYlYlYlVlVlVlVlYvVlVYVlyYVYyYVlVvVYVYVlyYVlVlVlVYylylVlVYVlVYVYyYVlVYyYylyVVYVVVYyYVYVlylylyYVYylVVVYylVlyYyVylVVylylVYyYyYyVyYyVyVyVyYyVyVyVyVyVyy
YlVvYvVvYlVllvlvYvYvVlVlVlYvYvYvYvYllvVlYvYlYlYvlvYvlvYvlvlvYvYvYlYvlvYlYvYvYvYvVvYvYYllvv;:vG8B888B8B8ZV;NB888B8B88c;vj8B8B8B8Bc;YB8      B8B880                .:v;.;,. ,yLllvYvlvYvVvYvYlVvYvllVlYvVvlllvYlllYvYvYvlvVvYvlvlvYvVvYlYvYlllllYvYlYvVvlvlllvlvYvlvlvYlYvVvYllvYlYvlvllVvYvYvVvVvVvYlVvVvVvVlYvYlYlVvYlVvYvYvVvVlYlVlVlVlVlYlVlVlYlYlVlVlVlYlVvYlVlVlVYVlVlVlVlVlVlVlVYVlYlVlYlVYYlVlVlVlVlVVVlyYylVYVlyYVYYYylyYVlVVyVyVVlyYylVlVlVYVYyYyYVYVYVYVYylyYyYylVYyVVlVYVYyYylyVylVYVlVYylyYVVVlyYVYVYVVVYyVyVyVyYyYyVyyyYyVyVyVyYL
lYlVlYlVlYlVlYlVvVvYvYvllVvVvYvVvYvYlYvlvYlVlVlVlYvYvlvllVlVlYvYlYvYlYvVvYvlvYvYvYlVvlvv;;vgB8B8B8B8BBr:veB8B8B8B8c;;HB888B8B8j;;qB8y     B8B8            ;HH8B8B8B8B8BDrvvVlVvYlVllllvYvVllvYllvVvYvYlYvlvYvlvVvllYvVvlvYvVlYvYlYvlllvYlYlYvVvllYvlvYvYvVlVYVlVlVlYvYlVlVlYlVvVvYlVlVlVvVlVlYlVvVlVlVlVvYlVlVvVvYlVvVvVvVlVvVlYlYlVlYlYlVvVlVvYvVlVlYlYlVlVlVvYlVYVlYlVlVlYlYlVlVYVYylVYVlVlVlYlylyYylylVYVVYlYlVlVlVlVlVYVYVYVlVYVYVlVYVYVYVVylyYyYVYVVyVyYVVyVVlylyYyYVYVYVlyYVYylVYVlyVyYyVylylVVylyVyVylVlylyYyYyVylVVVVyYyYyVyVyVyVyVVy
YlYlYlYlVvVlVlYvVlVlYlVlYvYlVlVlVlVvlvYvlllvVlVlYvYlVvVlYlVvVlllVlllVlVvYllllvlvYvYvVvv;;H8B8B888B8B4:;C8B8B8B8BC;;TgB8B8B8Be;;m8B8B     v8b          vG8B8B8B8B8B8B8B8BOUlvvllvYvlvVlYvYlYllvYvYvYvYvYlYllvYvYvYvYlYvlvVlYlYvYvYlYlYvYlYvYvlvVlYvYvVlYlVlYvYlVvVlYlYlYvVlVvYlYlVvYvllYvYlVlVlYlYlVlVlYvYvVlYvYlVlVlYvVlVvYvYlVlYlVlVlVlYvVlYlYlVlYlYvYYylVvVlVlVlVlVlVlVlYlVlVlVlYlylVYVlyVyvVlVYylyYyYVlVYylyYVYVlVYVlyYVYVYVlVlVYylyYyVVlVYVYyYyYyVVYyVyYyYyYylVVyYylyVVVVYVVylyYyVVYyYyYyYVYVYyYVVyVVYyYylyYylyVyYVYylVYVVVVylyVyYyVyVyly
lYlYlYlVvVvYlVlYvVvVlVlVllvVlVvVvVlVlVlVlYvYlYlYvVvVlVlYvVvVlVvlllvVvYlYllvlllvYvYlvv;:hB8B8B8B8B80v;cB888B8B8Gy:V68B8B8B88r,rB8B8B8     M        .,B8B8B8B8B888B8B8B8B8BrvvvvvlvlvlvYlVlllllYllvYvllYvYlYlYllvllVvVlVlVlVllvVvYlVlYvYlYvYlYvYvYvlvllYvVlVvVvYlYvVlYvYlVvVlVvVlVvYlVlYlVlYlVvYlYlVlVlYvYlYlYlYvVlVlVlVlVlYlYvVlVlVlYlVlVlVlVvYlYlVvVlVlVlYlYlYlVlVlYlVlYlVlVlVlVlVlVlVlVVVvVlVlVYVlyYylylyYylylyYyYVYyYVYVlylVYVYVYVlyYyYyVVlylVYVYyYyYyVVVylVVyVyYyVylyVyYyYyYyVyVyYyVyVyVyVyVyVyVyYyYyYyYVYVYyYVVVYyYVYylYlylyVylylyVyYyVVV
VlVvVlVvYlYlYvYvVYYlYYVvVlVvllVvVvYvYlVlYlYlYvVlYlVvVvVlVlYlVllllvllYvVvYlllYvVvYvv;;vg88B8B8B8Bq;:k8B888B8B8y:vgB8B8B8B8M,;eB8B8.              vB8B8B8B8B8BeO8B8B888BGv;wOr;;vvYlYvVlVlVvlvYvlvYlVvYlYvlvYlYllvlvYlVvllVlVlYvYllllvVlVvlvlvYvlvYvVvYvlvYlYvVvYvVlYlYvVvVlYlYvYlVlYlVlVlYlYvYlVlVlVlYlYlVlVlVlVlVvVvVlYvVvYlYlVlYvYlYlVlVlVlVlVlVlYlVlVlYlYlVlYlVlYlVlVlVlVlVlVlVYVYYYVYVlYYVYylYYVlyYVYVlVlVlyYVYVYyYVYVlylVYVYylyYylylyVyYyYylylVYVYVYyYyyyVyYyVVVyVyVyVyVyYyVyVyVyYyVyVyVyVyyyVyVyVyVyVyVyVyVyVyYyVyVyVylylyVyVVlyVVYyVyly
lVvVlVlVlVvVlYlVlYlYlVlVvVlYvVlVlYlYlYlYlYlYlVlVvYlVlVvYlYvYvYlVlYlYlVvVlYlVvVYlvv:;b8B8B8B888Cv,vB8B8B8B8B4,;j8B8B8B8Bm:;@8B8B8.            .j8B8B888B8B8OZG88888B8Dv:yD888DkvvvvllvYvlvlvllYlVvYlYlYvYvVlYlVlYvYvYlVllvlvYlYvlvlvVlYlVlVlYlYlVvlvYvYvVlVlVvVlYvYlVvYlYlYvVlVlYvVvVvYlVlYlVvYvVvVlYvylVlVvVvVlVlVlVvVlVlVlVvVlYlYvVlVlVlYvVlVvVlVlYvYlVlVlVlVlVlVlVYYlVlVlVlVlVlVlVlVYVYVYVYVlVYyYVYVYyYVYVlVlVYyVVYyYVYVlylVYylVVVlVYVVyYyVyVyVyVyVyVyVyVyVyVyVyVyVyVyYyVyVyVyYyVyVyVyVyyyVyVyVyVyVyVyVyVyVyVyVyyyVyVLVyVyVyYyYyYVYVYyVyVVy
VvYvYvVlYlVlYlVlVvYlYlYlVlYYYlVlVlVlVvVvYvYlVvYlYlYlVlVvVlYvVlYlVlYvVvYvVlVvVvvv;;UB8B8B8B8B9v,vBB8B8B8B8t::tB8B8B8B8j::CB8B8B8BM          ;8B8B8B8B8B8BOtGB8B8B8BZvvX8B8B8B8BcvvvvlYvYlYlYvYvYvYvYlVlVlYlVlYlVlVlVlVlVlVlVlVlVlYlYlVlVlVlYvYlVlVlYlVlYYVlVlYlYlYlYvYlVvYvVvYlVlVlVlYlYvYlVlVlYlVlVvVlVlVlVvVlVlYlVlYlVlYlYlVlYlVlVlVlVvYlVvVlVlVlVlVYVlVlVlVlYlVlVlVlVlVlVlYVVYylyYVlyYVlyYVYVlVYyYylyYyYylyYVVyYyYVlylVlylVYVYVYylyVyVVVyYyYyYyVyVylyVyYyVyVyYyVyVyVyVyVyVyVyVyVyVyVyyLVyVyVyVyVyVryyyyyyyyVyVyVyyLyLVyyyVyyyVyVyVyVyVyVyly
lVlVlVlVlVlYlVlVlVlVlylVYVlYvYlVlVvYvYlYlVlVvVlVlVlYlYlVlYlYlYlVvVvYvVvYlYlYvv;;vBB8B888B8eU:vg8B8B8B88Dv;l8B8B8B88O;:X8B8B8B8B8Gy          y888B8B888pbG8B8B8B8BL;k888888B8c;;Nv;vvvlvVvYvYlVlVlYlVlYlYlVlVlYvVlYlYlVlVlVvVlVvYvVvYlVvVvYvlvYvYlVvVvVlVlVlVlVvVvYlVlVlVlVlVlYlYvYlYvYlYlVlVlVvYlYlYlVlYlYlYlVlYlVlVlVvVvVlVlVllvVlVvYlVlVlVlVlVlVlVlVlVlVYylVlylVYylVYVlVlVlVlVlVlylVlVlVYVlyYVYylylVVVlVlyYyYVYyYVYyYyYVVVYyYVVyVyVyVyVyYyVyVylyVyYVVyYyVyVyVyyyVyVyVyVyVyyyVyVyVLVyVyyyYyVyyyVyVyVyyyyLVryyyLyryLyryrVryryyyyVyVyYyYyVyVVV
VYVlVlVlVlVlYYVlVlVlVlVlVlYlVvVlVlVlVlVlVlVvYlYvYlYvVlVlVlVlYvVlVlVlYvVlVlYv;:vc888g8B8B8m;,tB8B88888Cv.ve8B8B8B8ev,Te888B8B8Bkk8;  .,v.     M8B8B88OcO8888B888r;k8B8B8B8Bc,.v888T;vvvllVlYlYlYlYlYlVlVvYvVvVvYvVlYlVlVlVlYlYlVlYlYvlvlvllVlYvYlYvVlYvVlVlVlYlYlYvVlVlVlYlVlVlYvVlVlVlYlVlVvVlVvVlYlYYVlYlVlVvYvYvVlVvVlVlVlVlVvVlVlYlVlVlVlVlVlVlVvVYVlVvVlVlylVlylVYVYVlVlyYyYyYyVYlyYVlyYyYyVVYylyVyVyVyYylVVVYyYVYyVyYylyVyYVVyVyYyVyVyYyVyVyVyVyVLVyVyyyVyVyVyVyyyVyyLVyVyyyyyVLyyVyyrVyyLVLyyVyVyVyVLyyyryyyrVyyryryryryryLVryLVyVyyyly
YVlVlVlVlYlVlVlVYVlVlVlYlYlYvVlYvVlVlYlVlYlVlVlYlVlVlVlVvYlVlVvVvVlVlVlVlvvvvNB8BBB8B880v:v8B8B8B8BO;;yGB8B8B8B8V;ye88B8B8B8mvj8h    :  ::   .B888g0O888B8B8Bk;he8B8B888p;,AB8B8B8Br;vvlvVvllYlVvVvYlYlYlVlVlVlYlVlYvVlVvVvYlVvVlVlYvYvYlVvVlVlVvVlVvVlYlVlVlYvYlVlVlVlVlYlYlVlYlVlVlVlYlYvVvllVlVlVlYlVlVlVlVlVlVlVlYlVlVlVlVlVYVlYlVYyYyYVlVlVlVlVYVYylVYVlVYVlVlylVlVYyYVYVlVlyYVYyVylVYVYVlVlVYVVylyYVYyYylVVyYyVylyYyVyYVYVVVVyVyVyVyVyVyVyVyVyYyyLVLVyVyVyVyyyyyVLyyVyVyyLVyVyVyVyyyVyyyyyyyVyyyVyyyVyyyyLyryryyyryLyyyryLyyyyyLyyyyVyy
VYVlYlVYYlVlYlVlyYVYyYylVlylVlVlYlYlVlVlVlYlVlVYVlylVlVlVlVlVlVlYlVvVvVllvA88B8B8B8B8B8mDB8B8B8B8BBvwB8B8B8B8B8yTB8B8B8B8BO;vB8r          .  O8BBCeB8B888B8h:L8B8B8888j::y8B8B8B88Uvy;;;vvVvYvYlYlVlVlYvYlVvVvYlYlVlVlVlYlVlYlVlYlYlVlYlYlYlVlYlVlVlYlYvVvVlVvVlVlYlVvVYVlVlVlVYVlVlVlVlYlVlVvVlVYVVylVlVlVlVlVlVYVlVlVlYlYYYlYYVYylVYVlVYVlVYVYylVlVlyYVlVYylylyYylVYyYVlVVVVyYyYyVVYVYyYVYVVyYylylyYVYyVyVyYyVyVyVyVyVyVyVyVyVyVyVyYyVyYyVyVyYyyyVyVyVyVyyyVyVyVyVyVyyyyyyyyyVyVLyLVLyyVLyyVyVLVyyyyyVyVyyyVyVLVLVyVryryryryryryryryryLyLVr
VyYVVVlVVylylyYylVlYlYlVlVlYlYYVlVlVlVlVlVlVlVlVYYlVlVlVlVlVlVlYlVlVlYYlvYvyLkUHt0CpHHAUCG9BCvk#t8B89jCpUc6888CcjCmjbBB8BmvHB8B          ,ymB8B8B8B8B888BOycB8B8B8B80v:X88B888B8M,;8B8rvvvvVlYlVlVlYvVlVlVlVlVlVlVlVvVvVlVlllYlVvYvVlVlYlVvYlVvYlVlVlYlylVlYlVYVlVlVlVlVlylVlVlVlVYVlVlVlylyVVYylVYVYVlVlYlYlVYVYylVYVVVlyYylVYVlVYylVlVYVYVlVYylVYVYyYyYVlyVVYVlVVVYVYyVylVYyVyYyVyVyYVlyVyVyyyyyVyVyVyYyYyVyVyVyyyVyVyVyVyyyVyVyYyVyVyVyVyVyVyVyVyyyyyVyVyYyVyyyVyVyVyVyVyVyyyyyyyyLyyVyyyVyVyVyVyVyVyVyVyVyyyVyVLyyVyyryyyryryryLyLyLyryyy
yYVYVlYlyYylVYVlyYVVVYVYyVVlVVylyYyYVlYYVlYlVVYlVlVlVlVlYVyYylVlVlYlVYYVyllv;;;:;,;:;,;;;;vv    :B8BV  ,,,;v;;Ylv,;,,;lvvVVvAD          .B8B8B8B8B8OGB8BZk0B8e9B8B8tl48B8B8B8B8ZtB8B8B8B8wvvVlVlYlYlVlVlVlYlVlVlVlVlVlylVvVlYvVlVlVlVlVlVlYlVlVlVlVlVlVlVlVYVlVlVlVlVlVYylylVlVlVlVlYlVlYYVYVlVlYlVYyYyYyVylVYVYVlVlylVYVYyYyYVVylVVVVVVVVylyVVVyVylVYVYVYVYyVyYVYVYVYVVyVyYVYVlVYyYyVVVyYylyYyVyYyVylVYyVyVyVyVyVyVyYyYyVyVyVyYyYyVyVyVyyyVyVyVyVyYyVyVyVyVyVyVLyrYyVyVyVyyyyyyyVyVyVyVLyLyyyyVyVyVyVyVyVyYyVyVyVyVyVyyryLyryLVryLyrVryLyrVr
lyYVlylVlVlyYVYVYVYyYylVYylVYVYyYVlVlVlVlVYVYyYVYVlVlVlVlYlVlYlVlVlVlYlVlVlYvVvvvlvlvYlVvVh:   ;#8B6eb. :YYAyv;yyv;ylvLyvvyYVv          U8BBX2B8DCchAyvlMTXXv;.V68B8DOB:    kHYlM0bb0DDcVvvVlVlVlVlVlVlVlVvYlYlVlVlYYVYVlVlylVlVvVlVlyYVlVlVlVYYlVlYlVlVYyVylylVlVlylVYVVVlVlVlVlVYVlVlVlylyYylVYVYylVlyYVlVlVlyYVlVYVYylylVVylVYVYyYVVyYVlVvylylVYyVVYyVylyVylVYyYylyVyYVVyYVVVlVYVVVVVYVYyVyVyYVYyVylylVVylyYyYyVyYVYyYyVyVyVVVyVyVyVyVVYyVyVyYyYyVyYyVyVyyyVyVyVyVyVyyyVyVyVyVyVyYyVyVyYyVyVyVyVyYyVyVyVyVyVyVyVyyyyyVLyyVyyryryLyLyryryyr
VVyYyYVlVYyYVYVlVlylVYVlVlVlVYVYVYylVlVlVlyYylVlVlyVVYyYVVVVVlVVVYVVVlyYVYVlVlylyYVvVyLlyy4    ;#08OcB8;  :vNXAvywv:yyrHcvlhH           GB8B8wXC6DbcctkvvvtXvMtqBB8B6BO     ,Yv;;;:,:,;;vvYlVlVlYlVlVlVlVlVlVlVlVlVlVYVlYlVvVlVVVlYYVYVYVlVlVYVlVYyYVYVlVYVlYlVlYYVVVYVVylYVyYVVVVyYyYVYVVyYVYVlylyYVYyYVlylyYVlyVyVyYyYylYlVlVYVlVlVlVlVlVlVlVlyvVlVvYlVlVYVYyvVlVYVlVVyvVlVYyYyYyYVYVlylVlVlVlVYVYyYVlVYVlVlyVyYyYyYyYVlylylylVlyYyYyYylyYVlVVyVVYVYyVyVylyYVVyVVYVlVVyYyVyVVYyYyYVlyVyYyVyVyVyVyVyYyVyVyVyVyVyVyVyVyyyVyVyVyyyVyyLyryryryA
VyVyYyVyYyYyYVYVYVYylVVVlyVylyVVVVYyVVYVYyVVVVlVYVYVlVYVlVlyYyYylVlVYVYyVyVVlVVylyVYYLYyVMU    ;H8B0j298B:   ykAvYrv vyrhNMN;     ;     e8BBG@rvVmHMhjjrv;:UB8BBG8B8B8       vkyvYvlvvvYYyYVlYlVlVlVlVlVlVlVlVYVlVlVlVlVlYlVlVYVYVlVvVlVlYlVlYlVlVlVlYlYlVlVlyYVlYlVYVlVlVlylVlVlVlVYVlVlVlVlVlVlVlVvVlYlVYVYVlVlVlVlVlYvVvVlVlVlYvVlVlYlVYVlYvVvYlVYVlVlVlYvVlVlYlVlYlVlVvVlYlVlVvYlVvYlYlVlYYVlVlVlVYylVYVlVlVlVlVlVlyYVYYYVVVYylVYVYVYyYVlVVyYyYVlVlVYyYVYVVVYylVlyYyYVYyYyYylyYyYyYVlVYVYVYVlylyYVlyYVVyYyYyYVVyYyYyVyVyVyVyVyVLyryryryyL
yVylVyyyVYVYyVyYyYyYylylyYVlVYVlyYVVyYVYVVyYylyVyYyVyYVVVYyYVVyYyVylVlVlVYyYVYVlVlVlyVlvyAr,    YB8#cbqO8B6;  .vVvlyv ;YyA9O:     ;     888HUCCv;vyV;: ,HMvt8B888B8B8v    .;  hAylllVlVYyYYvVlVlVYVlyYVlVlVlVlYlVlVlVlyvYlVYYlVlYlVlYlYvYlVlVlVlYlYlYvVlYYVlYlVlYlVlYvYlVlYlYlYlYlYvYlYlVvYlYlVvVvYlVlVvYvYlYlYlVvVvVvVvVlVlVvlvVvVlVvlvYvYvlvYlVvYvYvYvYlVlVlVYVlYlYlYlVvYlVlVvVlVvVlYlVvVlYlYlYlVlYvVlVlYlVlVlVYVlVlYYVlVlYlVlVYVlVlVlVlVlYlVlVlVYVYVlVlVvylYlVlyYVlVlVYVlVlVYVlVlVlVlylVYVlVlVYVYyYVlVYyYyYylVVVYVYylyYyVyYyVyVyVyVyVyVyVr
YyVVYVVVYyVyYyVyYVlylylyVVVyYyYyYVVVYVYyYyYyYyYVYVYyYyVylylVYVlyVVlylYlVlYVVlYvVlVlyVllYyk,..    tB8CCjqqBB8B#;:..;vvv  ,;Ac     :      G8DM,,vv;;,v4ZD8B8B8BeB8B8BA  .  vBY   UVyVYvlvVlYvVlVlYlYvYvVvVvYlVlVlYlVvVlYlYlYlYlVvYlYlVvVlVvVlYvVlVvYlVvVllllvYvYvYllvYlllVlYvVllvYvVlVvlvYvlvYlVvVvlvYllvYlYvllYvYvlvVvlllvlvYvYvllVllvlvYvYlVlYvllYvlvYvYlYlVvYlYvYvlvVlYvYvYvlvVvYvYlYlVllvllVllvYvYvvvvvlvlvYlVllvVvYlVlYlVvYlYlVlVlVlVlYlVlVlVlYlYlYlYlVlVlVlVlYYVlVlVlVlVlVlVYVlVlyYYYVlVlVlVYyYVlYYVlyYVYVlVlVYyYyYVYyYVVyVyVyVyVyVyVLyyy
yYylyYyYVYVVVYVYYlVlVlVlVlVlVYylVYVlYlVlVlVlVlVlYlVlYlVlYlVlVlVlVlVlVlVlVvYlVlYlVllVyvlVX; ;w.     C8BGCCDGO8B8BBtmv,:v  .;y     .     ;b;vYvHZ#BqAm8B8B8B888B8B2       XB8jY  lh;vvlvYvlvYllvlvYlYvYvYvVvVlllVllvYvVvYvYllvlvlvllYvYvYllvVllvYllvVvYvYvYvllYvlvlvYvYvYlVvlvlvlvYvlvlvlvlvYvYvlvlvYvYvYvVvYvlvlvlvlvlvlvlvllYvlvYvVvYvlvlvlvYvlvlvlvlvYvYvVvYlYlllYvVvYvYvlvlvlvYvlvVlYvVvlvvvvvvvvvYlyVyVVYlvvvllVvVvYvYvYvYvYlYvVvlllllllvVlYlYlVlVlYvVvVlVlVlVlVlVlVlVlVlYlVvYlVlVlVlVlVlYlVYylVlYlVlVlyYVVyVylVYVYVYylVYVVyYyVyYyVyVyVLYL
lYlVvVlVlVlYlYvVvVlVlVYYvYlYvVlYlVlVlVlVlYlYlVYyYyVylYlYvYvlvllVvlvlvlvlvYvlvlvvvYvYlvvV;  U88       v08B8B8B8B888B8gC@8B8Bb           8BCc988B8B888B8B888B8GA        ;8B8B8Y,  B8;vvYlVvYvlvYvlvlvlvlvlvVvlvYllvYlYvYvlvYvlvlvlvlvlvlvlvlvYvlvYvlvlvlvlvlvlvlvlvlvvvvvlvlvlvlvYvlvlvlvlvlvlvlvlvlvlvlvlvYvlvlvlvlvlvlvlvvvlvlvlvYvlvlvlvlvlvlvlvlvlvYvlvlvlvYvlvlvlvlvllYvYvlvYlYllvlvvvvvvlAAXhUNNMNMhLrr44mAvvlllvllYlYvVlYlYvllVvYlYvlvVvYvVvlvYvllYllvYllvlvYlVlYlYlVlVlVvVvYlYlVlYvYlYlYlVlVlVvVlVlVlVlVlVlVlVlVlVlVlVlVlyYVVVYyVyVyVyy
YlYlYlYlYlYllvlllvYvYvlllvYvlllvlvYvYlllVvyVylyVyVVYVlylVVVvlvvvllYlYlVlYYVvlvVlVvvvVvv;,v8B8B8:        ,;c#BB8B8B888B888B88  :  .    ;B8B8B8B8B8B8B8B8B8c,         vB8B8B8B8X  .B8v;;vvYllvvvlvlvvvvvlvlvlvlvvvvvvvvvYvlvlvYvlvlvlvlvYvlvlvlvlvvvlvlvvvlvlvlvlvlvvvvvlvlvlvlvvvvvvvvvvvvvvvlvvvvvvvlvvvvvlvlvvvlvvvlvlvlvlvYvvvlvlvlvlvlvlvlvlvlvvvlvlvlvYvlvlvvvlvvvvvvvvvvvvvvvvvvvvvyrhhk4NyYvYvvvv;v;;;vlUXyvvllvYvvvYvlvlvvvlvlvlvlvYvYvYlVvVlYvYlYvlvYvYlYvYvYvVlYvlvlvYvYvYlYvYlYvVvVlYlYlYlVvVlVlYlVlVvYlYlVlVlYlYlYvVlVlylylVlyVylL
vllYvVllvYvYlVvYvYvVlllVlVlVlYYVlVlVlYlyYVVVvvvlvvvlvlvllYlLVYvllYYVvlvvvlYVYyylvvvYvvVO88B8B8B8U              ,vvkvvB98B8BO  ;;:     B8B8B8B8B888eBk.            y8B8B8B8B8B8.  :B8lvvvvvvlvvvvvvvvvYvvvvvvvvvvvvvvvvvlvlvvvlvlvvvlvvvvvlvvvvvvvvvvvvvvvvvvvvvlvvvlvvvvvlvvvvvlvvvvvlvvvlvvvlvvvvvvvlvlvvvlvvvvvvvlvvvvvvvvvvvvvvvlvvvvvvvYvlvlvvvvvlvlvvvlvvvvvvvv;vvlvvvYlyvllYvllrwUwTyrvv;v;v;vv;;vvvvv;;vThYvlvYvlvlvlvlvlvYvlvlvlvlvlvlvlvlvYvlvllYvYvYvlvYvlvYlVvllYvVlYlYvVlYvYlllYlYvYvYvllVlVlYlVYVlYlVlYlVlYlVlVYVlYlVlVYyYVlylVy
YvlvYvlvlvlvlvlvlvlvlvvvlvYvYvYlYvVlYvYvlvlvlvlvlvlvYvlvvvvvlYyYVvvvvvYYVYVvlvvvvvYH088B8B8B8B8B8BU;                          :;;;;.  vXM;yv.:.                V8B8B8B8B8B888Bt;  vB8m;;lyylVYYvVlYvvvvvvvvvvvvvvvlvlvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvlvvvlvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvvvvvvvvvvvvvvvvvlvvvvvlvvvvvvvvvv;YVAMhh4AAyyrArrLAyyyryyvvvvvvvv;;;v;vvv;;;vvvvvvyrlvlvlvvvlvlvlvvvlvlvlvlvlvvvlvYvlvlvlvlvYvlvlvlvYvlvlvlvlvlvYvlvlvYvYlllVvVvVlVlYllvlvYvYvYlVvlvlvVvYvlvVlVlYvYlYlVlVlVlyYL
vYvlvlvlvlvlvlvlvlvlvlvvvlvvvlvvvvvvvlvlvlvYvYvlvYvlvvvlvvvYvvvlvVVVVAAhryvv;;vyc9G8B8B8B8B8B8B8B8B8B8mv                        MX                          v88888B888B8B8B8B8B8;  qB8gv:vLrvVlYvlvYvlvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;;vrmmUMAYVvvvlvlvvvvvv;vlvvvvv;vvvvv;vvv;vvv;;;vvvvvvyVlvvvlvlvlvlvlvvvvvvvvvvvlvlvlvvvlvvvlvlvlvlvlvlvlvYvlvlvlvYvlvlvlvlvYvlvlvYvlvlvlvYvlvlvlvlvlvVvlvYvVvYvVlllVlVlVlVlYvVYYY
lvvvvvlvlvlvvvlvlvlvvvvvlvvvYvlvvvvvvvvvvvvvllYvlvvvvvvvvvlvvvvvvvlYLVvvvvVVMhm988888B8B8B8B8B8B8B8B888B8B8@kv .                 Z                    ;YUB8B8B8B8B8B8B888B8B8B888B. 0B8Bj;,vyvvvvvvvvvvvlvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;vyhUkyYvvvvvvvvvvvlvvvvvvvvvvvvvvvvvvvvvvvv;vvv;;vv;vvvvyVvvvvlvvvvvvvlvvvvvvvvvvvlvlvlvvvvvlvvvvvlvvvlvlvlvlvlvlvlvvvvvlvvvlvlvlvlvlvlvlvlvlvlvlvlvYvlvlvlvYllvllllYvllVvVlYlVlVvY
vlvvvlvvvlvvvlvlvvvvvvvlvlvvvlvvvvvlvvvvvvvvvvvlvlvvvlvvvvvvvvvvvv;VyrVylVLq9BB82O98B8B8B8B888B8B8B888B88888B8B8B88BO82rv      vB8BV         ,.:l2B888B8B888B8B8B8B8B888B8B8B8B8B8B  8B8B8;:;yvllVvvvvvvvllvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;;vAhUrVvvvvvvvvvvvvvvvvvvvvvvvvvvvVlvvvvvvvvvvvvvvv;;;v;vvvVrlvvvvvvvvvvvvvvvvvvvvlvlvlvvvvvvvvvvvvvvvvvvvvvvvvvvvlvlvlvvvvvvvvvlvlvlvvvlvlvlvlvlvlvlvlvlvYvYvlllvlvlvvllvYvVlYlYvYvYl
vvvvvvvvvvvvvvlvlvvvvvlvlvlvvvvvvvlvvvvvvvvvvvvvlvlvvlVYvvvvvvvvvvlyyvyyAyUb8OMy9B8B8B8B8B8B8B8B8B8B888B8B88888B8B8B8B8B8B8B8B8B8B88888B888B888B8B8B8B8B8B8B8B888B8B88888B8B8B8B8B8BGB8B8B8v,;yVlvvvvvvvvvvvlvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;vAHwrlVv;vvvvvv;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;vvvvvvv;;vvvvVMvvvvvvvvvvvvvvvvvvvlvlvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvlvvvvvvvlvlvvvlvvvvvlvvvvvlvvvvvlvlvlvlvlvYvlvlvlvlvlvYvYvYvYlVvV
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvlvvvlvvvvvVMMylMZmp6Gh;vm88g8B8B8e888B8B888B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B88888B88888B888B8B8Bewv;vlvvlvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;lNHLlvlvvvvvv;vvv;vvvvvvv;vvvvv;vvv;;;vvvvvvvvvvvvvvvvvvvvvvv;vyTvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvvvvvvvvvvvvvvvvvlvvvvvvvvvvvvvvvlvvvlvvvvvvvvvlvlvvvlvvvlvlvYvlvYvYlvl
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvvvvvvvvvvvlvvvvrbXXl;r6B89eB8OOjOB8B8B8B8B8B888B8B8B8B8B8B8B888B8B8B8B8B8B8B8B888B8B888B8B888888888B8B8B8B8B888B8B8B8B8B8B8B888B8B8B8C;;Vvvvyvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;;rXhYllvvvvv;vvvvvv;;vvvvvvvv;;vv;;vv;;vvvvvvvvvvvvv;vvvvvvvvv;v;vVAvvvvvvvlYVVyYYvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvlvvvvvvvvvvvlvvvvvvvvvvvvvvvvvvvvvlvvvlvlvlvlvlvlvlvllVvl
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvvvvvv;vvvvY;;vvy4Yv;vApDBB8BGC9B8B8B8B8B8B8B888B8B8B8B8B8B888B8B8B8B88888B888B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B6e8B8B8B8B8B8B888B8C:;lvvvylvvvvvvv;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;A4rVYvvvvvvvv;vvvvvvv;lvvvvvv;vvYvvvvvvvv;vvvvvvvvvvvvvvv;v;vv;;vvlYyvvvllLyyvvvyyyvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvvvvvvvvvvvvvvvvvvvvvlvlvvvYvlvll
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvlvvvvvvvvvvvv;llvvylYvvvv#8GgB8egZ8B8B8B6BB98B8B8B8B8B8B8B8B8B8B8B888B8B8B8B888B8B888B8B888B888B8B8B8B888B8B8B8B888B8B8B866BeG88888B8B888B888U;vVv;vYvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;;;hHrlYvv;vvvvvvv;vvvvvvvlYvvvv;vvv;Ylv;vvv;vvvvvvvvvvv;vvvvvvv;;;;;;;vyrvYyy;.     .lAlvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvvvvvvvvvvvlvlvlvvvlvl
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;vyvvvl;VvyM9B8B888B8B8B8BeB8e8BOD698B8e8B8G8B8B8B8B888B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8B888B8B888B8BeB8B888B8B8eg9BB8B8B8B8B8B8B8v;vYvvvlvVvvvvvv;vvv;vvvvvvvvvvvvv;vvvvv;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;;;vlMNLVllv;vvvvvvYvvvv;v;v;vvvvvvv;vv:,vyyvvvvvv;vvvvvvv;;vvvv;vv;;;;;;;;vhA.           :VVvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvlvvv
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvvvvvvvvvvvv;vvyVv;vlvvv;VB888B888B8B8B8BB6eBe02OGB8j6BDH988B8B6B8B8B8B8B888B8B8B8B8B888B8B888B9qBe8B8O0G8B8BGB8B8B8B8B8B8B888B8GeB8B8B8B888B888;;YYvvvvvVvvvlvv;vvv;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;vvvvvvv;vvvvvvvvv;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;vvvvvvv;;;;lhXXAyvl;vvv;vvvvvvvvvvvvvv;;v,;VYvvvvvv,;vVVMrrYYvvvvvvvv;vvv;;vv;;;;;;;;;lA;  ,;:vv;     lyvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;vvvYl;;Y8B8B8B8B8B8B8B8B8889#G8B8@je8ChZe8g9BZ6@8OGB8BBOGB8B8B8B8889BBeGBBOO8B#OpO6#OgBG8B8B8B8B8B8B8B8B8B8B888B8B8B8B888B8B88#,VvvvvvvvvvvvYvv;lvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;v;vvvvvvvvvvvvvvvvvvvvv;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;v;;vLAUNLvllv;v;vlv;;;vvvvvvvvlvvv;vV.;yLvvv;vv, .;vvlYvvvVYVvlvv;;;v;;;v;;vv;vv;;vMl,, ,,.     .,;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlvvvvvvvvvvvvvvvvvvvvvv;vlY;vr8B8B8B8B8B8B8B8B8B8B8B8B88pc8GbpBO899BgtBBZ9OZ8B6Oge8B8B8pZBj#99pO8B988B8BeB888B8B888B8B8B888B88888B8B888B8B8B8B8B888B8B;;Y;vvvv;vl;v;vvvlv;vvv;vvvvv;vvv;vvvvv;vvvvv;vvvvvvvvvvvvvvvvv;vvvvvvvvvvv;v;vvv;v;vvv;v;vvvvvvvvvvvvvvvvvvv;vvvvvvvvvvvvvvvvvvvvv;;;vAkArVv;vvv;v;v:;v;;vvVvvvlvvvvvvvvlv :Vwryvv;v;. .,;;v;;;vvYlYvv;;;v;vvvvvvvvvv;:vMv      .:;;.  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvl
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvYvvvvvvvvlA;;vq88888888B8B8B888B8B8B8B8B8B8B8B8Be9B9eC8B8e8OeB89eBBB89BB#cgjGB8B8B8B8B888B8B8B8B8B8B8B88888B8B8B8B888B8B8B8B888B8B8B8B89;ylvvvv;vlv;vvv;vvv;v;vvv;v;v;v;v;vvv;vvvvvvv;v;v;vvvvv;v;v;v;vvv;v;vvvvvvvvvvv;vvvvv;vvv;vvvvv;vvv;v;v;vvvvvvv;vvvvv;vvvvv;v;vv;:;vwwTLlvv;v;v;vvv;;.v;:;vvYlYYvvv ;vvvvv: ,;vyULlvvv;   :;v;;:,:;vyYl;v;vv;;vvvvvvv;;;vLY ..;;;;. lvvvv;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvlv;v;vvvvAv;;UB888B8B8B8B8B8B888B8B8B8B8B888B8B8B8B8e0B8O6B8B889e8B8g#OO88B8B8B888B8B8B8B8B888B8B888B8B8B8B8B8B8B8B8B8B888B8B888B8B888B8;vl;vll;;vv;;vvv;vvv;v;v;v;v;v;v;v;v;vvv;v;v;v;v;v;v;v;v;vvv;v;vvv;v;v;;;v;v;v;v;v;v;v;v;v;;;v;v;vvv;v;v;v;v;vvv;v;v;v;v;v;vvv;;:YAXMyvv;vvlvv;v;v;;;;:v;;vv;VvVYlv  :vlvV;..:,;vNXAVVv;:. ..:;v;;:;;vvl;;;v;;vvvvvvvv;v;;ymY: . .;vYYvrvv;vvv;v;vvvvvvvvvvvvv;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
vvvvvvvvvvvvvvvvvvvvvvvvvvvv;vvvvvvvvvvvvvvv;v;vvvvvvvvv;vvl;vvvvvvrY;;le8B8B8B8B8B8B8B8B888B8B8B88eZ8B8B8B8B888B8BOB8B8B8B8e8B8G8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8BUv;;v;lv;;lv;;vvvvvvv;vvv;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;;;v;;;v;;;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;vv;;:;rMrVlvvvv;;vvvvvvvv.;;:;v;v;;vvvlVy.. vlvvV,.,:::.;vVyyvv;: ..,,;;v;,,;vv;;;;;vvvvvvvvv;v;vvVvvvvyVvyAmCv;vvvvvvvvv;vvvvvvvvvvv;vvvvv;vvvvvvv;vvvvvvvvv;vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv;vvvvvvvvvvvvvvvvvv
vvv;v;vvvvvvvvvvv;vvv;v;vvvvv;vvv;vvvvv;v;vvvvvvvvvvvvvv;;Yvvvv;v;lTv:,A8B8B8B8B8B8B8B888B8B8BmYlvXtjcUMktt08B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8BBB8B8B8B8B8B8B888B888B8B888B8B8B8B8B8B8B8B2;v;;vv;;vYv;;v;v;vvv;vvv;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;;;v;v;v;;;v;v;v;v;v;v;;;;;v;v;:;krYvlvvvvlv::vv:vvvv::;;;:;;vvvvvvyl,..;lvvV;..,:::,,,;vyvvv;.....:;;::,;Vv;;;v;;;vvvvvvvvvvvvrUmLymDbchV;;vvvv;v;v;v;vvvvvvvvvvvvvvv;v;v;vvvvvvvvvvvvvvvvvvv;vvv;vvv;v;vvvvvvvvvvvvvvvvvvvvvvvvvvvvv;vvv;v
vvvv;vvv;v;v;v;vvvvvvvvv;vvv;vvv;vvv;v;v;vvv;v;vvvvv;v;vvvvv;vvv;vVAv,:GB8B8B8B8B888B8B8B888Bvvyq8B8B8B8B8BBOm4bB8B8B8B8B8B8B8B8B8B8B8B8B8G9O8B8B888B8B88eCkAMyme8B8B8888888B8B8B8B8B8B888B8B8B8B8B8BM;;vvv;;v;v;vv;v;;vvv;v;v;;;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;;;;;;;;;v;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;;;;;;v;;;;;;;v;;;:vUVvllvlvvvv;v:,vv;;;v:;;;:;:;;;;vvvVv.:,,;lvlY; ..:;;:,,:;vlvv;,.   ,:;:,,vvv;v;v;vvvvvvv;;v;;v;vlkGqwyvv:;;v;v;v;v;v;v;v;v;v;v;vvv;v;v;v;vvvvv;vvvvvvvvv;v;vvv;vvvvvvvvvvv;v;vvv;vvvvvvvvvvvvvvvvvvvvv;vvvv
v;v;v;v;v;v;v;v;v;v;v;v;;;v;v;v;v;v;v;vvv;vvv;;vv;v;v;vvv;vlvvv;;lYvv.;B8B8B888B8B8B8B8B888BC,;vyp8B8B8B8B8B888GUAcB888B8B8B888B888B8BCHpZ8B8B888B8B8B8B8ZcyVvv;v88B8B8B88888B8B8B8B8B8B888B8B8B8B8B8r;,v;Y;;;v;;vv;v;;;v;;;v;;;v;;;;;v;;;v;;;v;;;;;;;;;;;v;;;v;;;v;;;;;;;;;;;;;;;;;v;;;;;;;;;;;v;;;;;;;;;;;;;;;v;;;;;;;v;v;;:.vMVvvyllvvvv;v;;;:;v;;v;:;;;;;;;;vvvvl:.::,:;lvvv, ..,:;::,:.vrlvv;;   .,:,,,vv;;;;;;vvvvvvv;vvv;v;;;Al:,;;;;;;v;v;v;v;v;v;v;v;;;v;;;v;v;v;v;v;v;v;v;v;v;vvv;v;v;v;v;v;v;v;v;v;v;v;v;vvv;vvv;v;v;vvvvvvvvvvvvv
;v;v;v;v;v;v;v;v;v;v;v;vvv;v;v;v;v;v;v;v;;vv;v;v;v;;;vvv;;lY;;;;;y;vr.y8B8B8B8B8B8B8B8B8B8B8U,;;;;;;vvvMH#e8B8B8B6HLVrC8B8B8B8B8eBCkUkjeB8B8B8B8B8B8B6jXvvvvvv;;;8B8B8B8B8B8B888B8B888B8B8B888B8B8B8BvYv;;vv;vv;;vvv;;;;v;;;;;;;;v;;;;;;;;;v;;;;;v;;;;;;;v;;;v;;;;;;;;;;;v;;;v;v;v;v;v;;;v;v;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;: vA,,vVvVvYvv;v;;;v;;v;:vv;;vvv;;,,vlvlv;.:,;:,;vvvv;  .,:;::,..;rVvvvv;. ..:,,vV;;;;;;;;;vvvvv;vvvvYv;;lv;:;;;;v;v;v;;;;;v;v;v;v;;;v;v;v;v;v;v;v;;vv;v;v;v;v;v;v;;;v;v;v;v;v;v;v;v;v;v;v;v;v;;;vvv;vvv;vvvvvv;v
v;v;v;v;v;v;v;;;v;v;v;v;v;v;v;v;v;v;v;;;v;vv;;v;v;v;;vv;;;Vv;;;;vYVLA:jB8B8B888B8B8B8B8B8B8BH:vvvvv;;::,;:;;vvAUttcMMlvvvLk0DmkwwU4MktGB8B8eeOtyvv;;;:;;;;vvvv;,vB8B888B8B888B8B8B8B8B8B888B8B8B888B86;wv:;;;;v;;;vvv;;;v;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;v;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; ,8BC;;;vvvvYvvvvvvvv;v;;;v;;;v:;:;,;vvlV,:,,,;:,,;vYv;  ...;,:,,.;VY;vvY;. ..,.vMv;vvvvv;;;v;vvv;vvv;vvv;lVv:;;;;v;v;v;;;;;;;;;v;;;v;;;v;v;;;v;v;;;v;v;;;v;v;v;v;v;;;;;;;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v;v
;v;v;;;;;v;;;v;v;;;v;;;v;v;v;v;v;;;v;;;v;;;vv;;;;;;vvv;;;vvvv;;vTMLv:;T8B8B8B8B8B8B8B8B8B8B8k;;vvvvvvv;vvvvvvlvvvVVrVyyyVyvlVyTNAXywrTkUMrvvvvvvvvvv;vvlvvvvvv;:C8B8B8B8B8B8B8B8B8B888B8B8B8B8888888Bg.Ayv;:;vv;;;;v;v;;v;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;. eB8B8Gmv;;vlYvvvvvvvv;v;v;v;;;;,;;;:vvvY;.:,:,;,,.;vvlv.  ..,:;,..vyl;;vV;:...,,wYvvvvv;vv;;v;vvvvvv;vvvlvvYy;;;;;;;;;v;v;;;;;;;v;;;;;;;v;v;v;v;v;;;;;v;;;v;v;v;;;v;v;;;;;v;v;;;;;v;;;v;;;;;;;;;;;;;v;v;v;;;v;;;
v;;;v;;;v;;;v;;;;;v;;;;;;;;;v;v;v;;;v;;;;;;vv;v;v;;;v;;;;;vvv;vXUv;:;vZB8B8B8B8B8B8B8B8B8B8Bc;;vv;v;vvv;;vYvyVryrVyYyYylVVyVLVLyLVyyryhMrVLlYvvvvvv;v;vvvvvvv;;v888B8B888B8B888B8B8B8B8B8B8B8B8B8B8B8B.vwvv;:;v;;;v;;;v;v;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;: LBBB888B8DXv;;lvYvvvvv;;vvv;;:;;;vv:;;vvvv..,.,:;:;,,vVvv:    .,:.,,AVv;vvl;, ,..vrvvvv;vvvv;;;;vvvvl;;;vvlvvvy;;;;;;;;;;;;;v;;;;;;;;;v;;;v;;;;;v;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;;v;;;v;;;;;;;v;;;v;v;;;v;;;v;v
;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;;;;;;;;;;;;;;vv;;;;;;v;;;v;v;vvyvy;v;::yB8B8B8B888B8B8B8B8B8B82v;;;;;;,:..   ,;YyLAyyylYvlvYlVvVYyYVVyrMAryrlyvv;;,::;;;;;;;;vv;;888B888B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8,,MV:v;;;v;v;;;;:;vv;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; ,886GOee8B8B8Hv;;vvvvvv;;;v;;:;;;;vv;:;;v;V;,.,..,;::,:vAlvv;   ..:,.;Avv;;vv;,.. ,ylvvvvvv;vv;;v;v;vvvvvvvvvvvlyl;,:;v;;;;;;;;;v;;;v;;;;;;;;;;;;;;;v;v;;;;;v;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;;;;;;;;;;v;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;;rTvv;;;,:v8B888B8B8B8B8B8B888B8Bbvvvv;:;VyvMGGcylvYVLrrVVvvvvvvvvvvllvyyrLLYyvv:        ,:;;;:;;v;mB888B8B888B8B8B8B88888B8B8B8B8B8B8B8B8B2,vAv:v;;v;;;;;;;:;v;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;, h8e8eeB8B8B8B8Bqv;;;vvvv;vvv;;vv;;;v;::vvvvV;. ...,:,:..lrvlvv;:   ...vUvv;vvv;,.. vVv;vvvvvvv;;;v;;;vvvvvvVlVvvvyVl;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;;;;;;;;;;;;;;v;v
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;;;:v;vlvvv:v;; lB8B888B8B8B8B8B8B8B8B8kvvlYvw8k  BB8B8BH;lvlVyvv;v;;;;;vvvvllVvVlv;VvAB8B8B8NAY;..;vvVvrB8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B:;vvv;vvvv;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; .GB8BBB8B8BBe888B80Y;vvvvvvv;;;;;v;;;v;;;;vvvl;, . .,:,,.:lyvvvllv;. ..:UNvvvvvV;,...Vvvvvvlvvvvvv;;;;;vvvvvvvvvvvvyVl;;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:;;;;;;;;:vvvv;vv:;;; A888B88888B8B8B8B888B8BX;;;vlTeBw,v688B8v ;wAvvv;v;;;;;;;;vvvvvlv;;wN;y8B8B8Bv.yO86LMmTyC8B8B888B8B8B8B888B8B8B8B888B8B8B8B888B8B8v:vYv,vv;l::;;;;;;;;;;;;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; vB8BBB8B8B888BBB8B8BCvv;vvvvvv;:;vv;;;vv;;vvvvVv;.  ..,,;.;rVvvvvvyV;   ;whvvvvvV;. .;ylvvvvv;v;vvv;v;;;vv;;;;vvvvvvvvvv;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:;:;;;;;:;;;;;;vvv;;v;,vv: XB8B8B8B8B8B8B8B8B8B8B8v;;:::,;vrryyNG6GAAOB8Av;;;v;v:,,;;v;vvvvv;GBM.,jG88B8Yvk8B8twVlv@B8B8B8B8B8B888B8B8B888B888B8B8B888B8B8B8Bv,,Ly:;;;vl;;,;;;;;;;;;:;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;. 48eeB8B8B8B888B8B8B8B8kv;vvv;v;;;v;v;;;v;;;vvvvlYv;,   ,,..yhyvvvvvyVv   .yAvv;vvlv, .;yvvvvvv;vvvvvvv;;;v;v;;vvvvvvvlvvvv:::;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;;;:;;;;v;;:Tv:vv  88B8B8B8B8B8B8B8B8B8B8Bv;;,:...,,:;:,;,;;lvv;;;;;v;;;....;;vvv;vvvC8pULrMrvUyyyyv;,;;;;Vt8B8B8B88888B88888B888B8B8B888B8B8B8B8B88OZ;.vVvv;;,vv;;:;;;;;:;;;;;;;:;:;;;;;:;;;;;;;;;:;;;;;;;;;;;;;;;:;:;:;:;:;:;;;;;;;;;;;:;:;;;;;;;;;;;;;;;;;;;;;;;:;;.l8B8B888B888B8B8B8e8B8B8#r;vvvvvvv;;;v;;;vvv;vvvvlvylv,.  .,.vLwVvvvvVVv;   yAvvvvlYv; .;Avv;vvv;vvvvv;;;;vvvv;vvvvvvlvvvlvvv;.;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:;;;;;;;:;:v;;:;:;;;,v;vv;Yv;;v, 8B8B8B8B8B8B888B88888BZv;:;::,,,:;;;;;;::;;.,,;;;;;;,   .,;;;;;;v;:.,:;;;:::;;v;;:;;;;vyOB8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B888B8B8B8v,vv;v;;,;:;vv;;:;:;;;:;:;;;:;;;;;:;:;:;;;;;;;:;:;;;:;;;;;;;:;;;;;;;;;;;;;;;;;:;;;;;;;:;;;;;;;;;:;:;;;:;;;;;;vvv:yXAvrO8B8B8B8B8B8BBB8e8B8Btv;vvvlvv;v;vvv;v;vvv;vvlvYvvlv.  . :vAyYvlvlvL;  .vrvvvvvlv;  ;Avv;vvv;vvvvv;vvvvvvvvvvvvvvlvvvyvvv;:,:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:;;;;;:;;;;;;v:;;;;;v;v;:vlv.v, B8B8B8B8B8B8B8B8B8B888Xv;:::,;,::;,;:;:;,,,,::,;;;::    .,:;;;;;;;::,:;;;v;vvv;v;;;;:;vkB8B8B8B8B8B888B8B8B8B8B888B8B8B8B8B8B8B8Bewy,v;v;;;;;;,;;v;::;:;;;;;:;:;;;;;:;:;;;;;:;;;:;:;:;;;;;;;;;;;:;;;;;;;;;;;;;:;;;:;;;:;:;;;:;:;:;:;:;;;;;;;;;;v;.           ;g8B8B8B8BBB8B8B8BOvvvvvvvl;vvvvv;v;vvvvvvvvlvvlV;,   .:lyyvvvvvV;. .;yvvvvvVl:  vr;;;vvvvvvvvv;v;vvvvvvllVVyyYvYlYvvvV;:.;:;;;;;:;;;:;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:;
;;;;;;;;;;;;;;;;;;;;;;;;;;:;:;;;;;;;;;;;:;:;;;;;:;;v;;,;;;;v;;vLv.v, 8B8B8B8B8B888B8B8B8B8BYv;::,,,:,:::,:::,:,::;:;;:.:.    ..:,;;;;;;;:::;,::::;:;;;;;;;;lH888B888B8B8B8B8B8B8B8B8B8B8B8B888B888B8B8#lh;v;;v;;.vv;,:;vv::;;;:;;;;;:;:;;;:;;;:;;;;;:;:;;;:;;;;;;;;;;;;;:;;;:;;;;;:;;;:;:;:;;;;;;;;;;;;;;;:;;;;;;;v;                ;8B8B8B8B8BBBBB8B84vvvvvYVvv;;vv;;;vvvvvvYvlvvvlvl;    ;VrlvvvvVv. .;VvvvvvYl. .yl;;vl;vvvvv;v;vvvvvvvvVYVlVYllyVVVVvylv:,,;:;;;;;:;;;:;;;;;:;;;:;;;:;:;;;:;;;:;;;;;;;:;:;;;:;;;;;:;;;:;:;:;;;;;;;:;:;;;;
;:;;;;;:;;;;;;;;;;;;;:;;;:;;;;;;;:;;;;;:;;;;;;;,;:;:v;:;;v;:v;;Yv;..;B8B8B8B8B888B8B8B888Bevv;;::,:,,,:,;:::;:;:;;;....,,.,.,,,,.   ,:;;;::,,.,,:,::;:;:;;;l@B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B88GcB:v;;vv;:;;:v:;:vl::;:;:;:;:;;;:;;;:;:;:;:;:;:;:;;;;;:;:;:;:;:;:;;;:;:;:;:;;;:;;;;v;v;vvv;vvv;vvvvvvv;v;;;,            ..    y8l,,lB8B8Bg98BeB8Zv;vvvvVvv;vvv;vvvvvvvvlvlvlvvvyl;   ,;yVlvYvVv, ,vLvvvvlV;. ;Mvvyvv;vvvvvvvvvvVllvvvYvVYyYyVyYVvVv;vy;..,:;:;:;:;:;:;:;:;;;:;:;;;;;:;:;:;:;;;:;;;;;:;:;:;;;:;;;:;:;;;;;:;;;;;;;:;:;:;
:;:;:;;;;;;;;;;;;;;;;;;;;;;;;;:;:;;;:;;;:;:;;;:;;;:;:;v;,v:;v;:vvv: U8B888B8B8B8B8B8B888B8mv;;:;::,:,,,::;:;;;:;;;,,.,,,;vvvvv;:.. . :;;;:::,:::,;:;,;;;;;;T98B8B8B8B8B8B888B8B8B8B8B8B888B8B8B8B888B8BtB,yv,;vv::;;;;::.;Yv,:;;:;:;:;:;:;;;:;:;:;:;:;:;:;:;:;:;:;:;:::;:;;:,;:;:;;;;;;;;:,,.                ,,;;;       . .;vvvvyV;     ,   lB8B89e6Ge8Zv;vvv;llv;vvvvvvvvlvvvvvvvvvlvvVl:   :vYVvvvyl: .vVvvvVvY: .Tv;v;vvvvvvvvvvvvvVvvvllVlYvVVyVyyLYVYVlVlv...;:;:;:::;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;:;;;:;;:;
;;;:;;;;;;;;;;;;;:;;;;;;;:;:;:;:;:;:;:;:;:;;;:::;:;;:;v;;::;v,;:vY; 888B8B8B8B8B8B8B8B8B88A;v;;:;:::;:;:;:;;;;;;;;;;vvrVYvyyryYVyvv;;;v;;:;:;;;:;;;:;:;:;;YO8B8B8B8B8B8B8B888B8B8B8B888B8B88888B8B8B8B8ZB.vy;,;;,;;,,:;;;,vv:,::;:;,:,;:;,;:;:;:;:;:::::;,;;;:;:::;:::;:;;;;;;;;;:;;;.                           ..  .. ..;vv;vvwy,        v;  v888egGeg88r:vvv;vvvvllvvvvvvvvvvvvvvlvlvlvrv, ..;vllvvVl; .;YvvllVv. vr;vvv;v;vvvvvvvvlvlvlvYvYlyyLyryyyVlrLyyMMy;. .,;:;:;:;:;:::;:::;:::;:::::;:;:;:;:;:;:::;:;:;:;:;:;:;:;:;;;:;:;:;:;:;,;
:;:;:;:;:;:;:;:;:;:;:;:;;;;;:;:;:;:;;::;:;;;,::;:;;;:;::;v,;vvv;.;;:B8B8B8B888B8B8B8B888B8vv;v;;;;;;;;;;;;;v;;;;;vvlLUD8kYlyYYr8BpywAyv;;;;;;;;;;;;;;;;;;vrBB8B8B8B8B8B8B8B8B8B888B8B8B8B888B8B8B888B8Beqv:l,;:v;v;;:;:::;,;;;,:,:,;:;:;:;,:,:,::;:;:::;,:,;::,;:;;;;;;;;;:;;;:;;v;.                             ,,;;;::,vv                 ,V.  GB8ee96688b;vvv;vvvvlvvvvvvvvvvvvvvvvvvvvvVVv.  ,;yVYvVyv  ;lvVvVv. vhv;v;v;;vvvvvvvvvvvlvVvVlylyYyVVYrllYyYyVyM4Lv. .::;:::;:;:;:::;,;:::;:;:;,;::::,:,;,:,;,::::;:;::,:,;:::;:;:;:;:;:;::;
;:;::,;:::;:::;:;:;,:,;:;::::,::;,:,:;;,::;:;;:,;;:;;:;,;;;;:;v.,.:l8B8B888B8B888B8B888B8Bv;vv;;;;;;v;;;;;;;v;v;;;v;vvv;vlrAArrrAyVvvv;;;;;;;;;;;;;;;;;;vYhZ8B8B8B8B8B8B8B888B8B8B8B8B888B8B8B8B888B8B8cgv ;;,.;v;,.::;;;,..v;:,:,:,:::::,:,:,:::,:,:,:,;:;:;;;;;;:..            ,:                             .;vvllvVhC;        .v;,       rA  kB8g6Oeee8O;vv;;vvlvv;vvlvvvvvvvvvlvvvvvvvlly;  .;vYylyyv  ,vYVvV. .Myvvv;;vvvvvvvvvvvYvVlVlyVyYVlVlyyyYyVyYVrArwhT;. .,:,::::;:;,:,:,:,;::::::,:,;:;:;:::;:::;,::;:::;:::::::::;:;,;,;:;,;
,;,;:;:;:;,;::,::::::;,;,:,:,::::;::::,::;::,,,;;;;;:;::::;;:;v,., ZB8B8B8B8B8B8B8B8B888B8vvvvvv;v;v;v;v;v;vvvvv;;;,.  ..:;v;;,, ...,;;;;;;;;;;;;;;vvv;vlyUOB8B8B8B8B888B8B8B8B8B8B8B8B8B888B8B8B8B8B888Bk vVv;;;v:,::::,;;,,;;:,;,:,:,:,,,:,,,:,,,,,:,;:;;;;:                                                   ,ymUj#8BH           :vv.      VU;,6B8BGG998Be;;vv;vllvvvvvVvlvvvlvvvvvvvvvlvvlyv;  .;vyyVVv  .vlyyv. ;r;vvvvvvvvvvvvvvYvlvYvYVyYVYylyVyVyVryyVyyryrrUTv  .:,;,:,:,:,:,:,:,:,:,:,:,:,:::,:,:,:::,:,:::,:,:,:,::;,:,:,;:::;:::
:,:,:::,;,:,:,:,:,:,:,:,:,:,:,:::,:,;,::;::,,,;;::;.,:;,;,;,,;;:,  g888B8B8B8B8B8B8B8B8B8BvvVvvvvvvvv;vvv;vvvvvvv;;,.              .,,vvv;v;;;v;v;v;vvYYVTCB8B8B8B8B8B8B8B8B8B888B8B888B8B8B8B8B888B8B8B8X ;L::.:;;;,,:,:,:::,;,,:;::,:.,,,,:,:.:,:,,,;;;,,                                                   . ..vXqOH:    ,;;.      .vy;,.    vpYC8B8B696DG89vvvv;vlVvvvlvvvvvvvvvvvvvvvvvlvvvYVv:  .:vlyrl,  ;vLy; .Vvvvvvvv;vvvvlvlvlvVlVlVYylVYyVyVLyLyyVyVLyryryXmU;  .,:,:,:,;,:,:,:,:,:,:,,,:,:,:,:,:,:,:,:,:,:,:,:,:,:,:,:,:,:,:::,:
,:,:,:,:,:::,:,:,:,:,:,:,:,:,,,:,:,:,::;.:,:;;:;;:.:;;.:::;,.;;Y  :8B8B8B8B8B888B8B888B8B8Uvvlvlvvvvvvvvvvvvvvvlv;::,vAcmUl;vA4hyv..,:;vvvvvvvvvvvvlvlLLYc#88888B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8  v: ;,.,;,,,:,,,:,:;v..,;;:.,,,,:,:,,.,,::;,.                                                     .,;:;;,;.        ,v;       vwv;,,   ;BeB8B8Be99O8B8lvvvvylYvvvvvvvvvvvvvYvlvvvvvvvlvvvyYv.  ,;VMr;.  vMV, VLvvlvvvvvvvvvYlYlylVVVYyVyYVVyVyVyVyVyVyyLyyyryA4bmL;;::,,,:.:,:,:,:,,,:,,,,,,,:,:,:,:::,:,:,:,:::,:,:,:,:,:,:,:,::::
:,:,:,:,:,:,,,,,:,:,,,:,:,:,:,:,:,:,,:;,:,::;:;,,,:;;,,::,;:;:;;, bB8B8B8B8B8B8B888B8B8B8Be;vvVlVvlvlvvvvvlvvvYvvvyN8B888B8B8B8B888brvv;vvvvvvvvllYvyLryVHZB888B8B888B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8Bk ;v,:;.,;;,;:;:;::,:vv,:,::;.,,:,:,,,:,;:,                                                       ,,;:,,.             vv.      vhlv;;.  vB8B8888eO6GB88vvlYvlvVvvvlvlvvvvvlvvvvvvvvvvvvvvvVYVv;. .vyXY:  :Av.;TvvvYvvvvvvvYYyYVlVlYlVlVYylyVyvlYyYyVyyyVyVyyryrkCv  ;;;,,,:,,,:,:,,,:,:,:,:,:,:,:,:,:,:,:,:,:,:,:,:,:,,,,,:,:,:,:,;
,,,,,:,:,:,:,:,:,:,:,:,:,:,:,,,,,,,::;,:,:,,,;,,,,:;,::::;;;.::: vN888B8B8B8B8B8B8B8B8B8B8B4vlvVYVlYvYvYvlvlYyVA@8BqA;           ;r9B8ZUrlvYvvvllYvVrMYr4b68B888B8B8B8B8B8B8B8B888B8B888B8B8B8B8B8B8B8B8B8B; v:,;;,;,,.........;v:::,;;;::::,:,:,:,.                                                      ...:.          ,       vv:.     Y4lvv;,  HB8B888B6OeGeBevvlvvlyVvvvvvvvvvvvvvlvlvvvvvvvvvvvvlylyv,  :MNv. .lv,vT;vvlvvvlvVlylYlVlVlVlyYVVyVyVLylvyYVVyyyVyYyyMHU      ;v;,.:,,,:,,.:,:,,,,,:,:,,,:,:,:,,,:,,,,,:,:,:,:,:,:,:,:,:,,:
,,,,,,,,,.,,,.,,:,,,,.,.,,,,,,:,:,:,:,,,,.,,:.,,,,;.,::.;;:,:;::.;TB8B888B8B8B8B888B8B8B8B8BY;vvVlVlVYYlVlVlyyryAYv,::;;vvvvv;;,,,:,;ycmTVYvYvyVVvVyArVAjjBB888B8B8B888B8B8B8B8B8B888B8B8B88888B8B8B8B8B8B8m  ..,  .            .      .. ,,:;;,:..                                             .   . ....,,:,   .        ;.     ,Av;,.   ,wUvlvv:  8B8B888eG9ee8BevvvlvVYVvvvvvYvvvvvlvlvlvvvYvvvlvvvvvlvVyV:  vry;  vvvrv;vvv;vvYvYlYlVYylllYlylyVyVyVyyVVyYyVyyLyyLAH2         ;;;:,,,,,,:,,,,,,.,,,,,,,,:,:,:,:,:,,,:,:,:,,,:,:,:,:,:,:,:
,,.,,,,,,:,:.,,,,,,,,,,:,:,,,,.,,,,,,,,,.,,:,:.:,;,:::,:;:.;;.:,,YB8B8B8B8B88888B8B8B888B8B88v;llVlyVyVyVyyLyyYlvv;;vVAUHckcccmjHUArvvvvvllyyrVyyryrAAyUcte888B8B888B8B8B8B888B8B888B8B8B888B8B8B8B8B888B8B8v    .                            .,,,.                                        . . ...,:,...,,.:.:,..,,.       v;;,.  ;Xlv;,   vmXlVVl; b8B8B8B8BBeBe8BevvvvvyyYvvvvvllvllvvvvvvvYvvvlvlvvvvvvvYVAY: .vTv. :vvyYvvvvvyVVvYlylVlVVyVylyYyVyyVVyyyyryyVrLAAwH#            .;;:,,,,.,,,,,.:,,,,,,,:,:,:,,,,,:,:,:,,,,,:,,,:,:,:,:,,,
,.,,,,:,,,,.:,,,,,,,,,,,:,,,,,,,,,,,,,,,,,,,,,,,;,,:,,:,;.:::.: ;B8B8B8B8B8B888B8B88888B8B8B8BUvYYVYLyyyryLyyYlvv;vvvvyyyyrrwrMAhAMyvvvvlvlVrVyLryAAAANmjc8B888B8B8B8B8B8B8B8B88888B8B8B888B8B8B8B8B8B8B8B8B8;;.  .                             ,..                                     . . ......... ,...;:..   yr .      ,y;;;:  ywvvv;:..LOAlyyl v88B8B8B8BBG9e8BO;llYvlvvvvvvvYvlvvvvvvvvvvvYvlvVvllvvvvvlyyy;,;lv, :vYYlvlvYyrVVYVyVlVYyVyVyVyYyVLVyyLLyVryyyyLwCj.              ,;;,:,,,,,,,,,:,,,,.,,,,,,,,:,,,:,:.,,,,,.,,,,,,,,,,,.:
,,.,,,.,,,.,.,,,.,.,.,.,,,,,,,.,,,,,,,,,,,.,,,.:.,,,,,,:,,;,.: .j8B8B8B88888B8B8B8B8B888B8B8B8BervyLyMrryryyVVvvvvvv;;;;;vvvvvvv;vvv;;;vvVVyVyyALLrmwUc#p0B8B8B8B8B8B8B8B8B88888B8B888B8B8B8B8B888B8B888B888mv.v,  ..                          :,,..                                     ... ... .   .:,,;.     ;p6, .      ;yv;;; :XrvLyy;:v9bTyAy;m8B8B8B888BeOeB8BCvlvvvllyvvvvvvvvvlvlvyllvlvvvllVvYvlvvvlvyVyv;:yY: ;YYVyvVyyVyVVlyVyVyVLYLyyVyyryLyAyrlyyLVyyApc                  .;::,,.,,,,,,,,,,,,,,:,,,:,,,,,,,,,,,,,,,,,,,,,:,,,.,
,.,,,.,,,.,.,.,.,,,.,.,,,,,.,.,.,.,,,.,.,,,.,.,,,.,.,.,,:,,..;;v8B8B8B8B8B8B8B888B8B8B888B8B8B8B86TvVyLyryyVlvvvvvv;v;;;;;v;vvv;;;;;;;vvvvYVLVrLArXkkck0D#BB8B8B8B8B8B8B8B8B8B8B8B888B8B8B88888B8B8B888B8B8BeO,,;,..,                          ;;.. .                                      .,,, ,,:vv;.     ;,.  ,mr ,;,;:. .lyvv;; LHyVwrY;;cGUUkwV8B8B888B8B8egO8B88UvYlllyVVlvvYvvvvvYlyYllylvvlvlvlvlvlvvvlvllYvyYlV; :lyVyVyVVVyVyyryryryyVLyLVyVLyyLAyLyrlyyhCk                     ;;:.,,,,,,,.,.,.,,,.,.,,,,,.:,,,,.,.,.,.,,,,,,,.,.:
.,.,.,,,.,.,.,.,.,.,.,.,.,.,.,,,.,.,,,.,.,,,,,.,.,.,.,.:,...;;,8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8BBrvvVVLVyvlvvvv;;;;;;;;;;;;;;;;;;;vvvvvlyVVVLyAXH4jmqZgB8B8B8B8B8B888B8B8B8B888B8B8B8B8B8B8B888B8B8B8B8B8B8v;;;:  ,   .                     ;;:.. .   .                           . ....,...;;;:,      ..     .m8v.vyvYv;.;ryvlv,;bHrrNrlvkZmUXyOB8B8B8B8B8B8BDgBB8BvvYlyvYYYvlvvvvvlvYVVvVlyvvvlvYlVlvvlvlvVllvllYlryv ;LLyyyyVyYLVyYLVryyYyVLyrVLyyyrrryAyyrT#h                        :..,.,.,.,.,,,,,.,,,,,,,.,,,.,.,.,,,.,.,.,.,,,,,,
,.,.,.,.,.,.,.,.,.,.,.,.,.,.,,,.,,,.,.,,,.,.,.,.,,,.,,:::,,;  vB888B8B8B8B888B8B8B8B8B888B8B8B8B8B8888qyvvLVVYvvvvv;;;;;;;;;;;;;;;v;vvvvllyVyVrLArkkUtC2qB8B8B8B888B8B8B8B8B8B8B8B8B888B88888B8B888B8B8B8B8B8j8;.;;: .,                        ,v;:...... . .                   . . .,,.. . .:;.            .;,.;yOBG.vyyvlv;:AwyVVv:yOUMhwVlMjH4rcB8B8B8B8B8geeeZ99eB8OvvylvllvlvvvvvvvYlVllvVlVvlvlvVyYvvvlvYlyvlvVlYlrrl,;VyyyVyVyyrVVVyyyyLVyyryAyryrLALLVrLU0y                         ;v,...,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,,,,,.,.,
.,.,.,.,.,...,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,,,,:,,..;:  B888B88888B8B888B8B8B8B888B888B88888B8B888BNvYVyyyyyVlvlvvvvvvvv;vvvvvvVVyyAAMLAAwrrMkhUjjp8B8B888B888B8B8B8B8B8B8B8B888B8B8B8B8B8B8B888B888B8B8Br :;;;:.,   .               .   ,;v;,.,...... . .   . . . . . ..,.... .   ;vl;     .        ,VLvrYAk8B..vyYYVv;lcrLryvYbj4hMAlrXmUcB8BOG8B8B8eeG9DGqZOGB8HvvvllvlllvvvvvllYvVlVlYlVlYvvvylVlVvVlVYylVlVVyYyyy;;vrVyVyVyVyyyVyVryyVryrLryArArrLrMCZ,                         ,Hv ,.,.,.,.,.,.,.,.,.,.,.,.,,,.,.,.,.,.,.,.,.,..,
,.,...,.........,.,.,.....,.....,.,.,...,.,.....,.,.,.. .;.  k8B8B8B8B8B8B8B8B8B8B88888B8B8B888B8B8B8B8B8B82cUrLArTTTwALryryrrMrArwAwMXwkHUrMrArLANrrN2B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8888888B8B8B8B8B8Oy :v;;.:.       .               ;v;;:,:.,.,...... .......,,....,:.  .,:,;;:     .       ;;vVVvvvlTBB8,.vLVyVvvlXwLAylvT2UAMyVVAh,v8B8ADO888B8gGqZ@Z#@bOB8vVlyllvyvvvvllllvllyyVvVVYvYvvvVvVlYvylVVYlyVYlyVVyrvvYylVlyVyVyyyyyVyyALryLyrrrLMrXtOr                          .Xv ..,...,.....,...,...,.,.,.,.,.,...,.,.,.,.,.,.,
.,.....,.,.,.,.,...,.....,.,.,.,.,.....,.,.,.,.........,;,  X888B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B82j0OcAyrhrAHhkUUNkUU4U4UXXMXNrVLVyYlvrXje8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8888888B8B8B8B8B8B8B8B8B8B8BA.;:vv,,     .  .              ;vv;;;,:,,,,.,.....,.,,:,,.....,...,,.,;     .     :VhyrhMywv;vNB8B8p ;yVryVvvvXNALLlvVTATryVVve..B8w ;9BBB8BeD##qpCjCCe80vrlvvYVyllvvvlvlvllylvvYlylYvlvllylVlVvVvlYVvllVyVYLyyyylyVyVyVrVyyLyrrArryrrryMAHZH.                          ,Xv ..,.,...,.,...,.,.,.,.,.,...,.,.,.,.,...,...,..,
....,.,.,...,...,.,.,.....,.,...,.,.......,.,...,.,...,::  ;8B8B88888B8B8B8B8B8B8B8B8B8B8B8B8B888B8B808B8B8B8w;YlB#0twUrAYyVrrAyMrrVylVVv;v;vYTt8B888B888B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B888B8B888Bv.v;v;,;.   ,       ,..       ,vYvv;;:;::::::,::;;;;. ....:,..:,:,.    :.    :vvyAUXyvLvllXG8B8B8h.,lVyyLYlvvrNrrVVvvVAyrhHXe8r;6Bh   ZB8eegG#0j2j2jbb9BlYyvvvVyyllvVvlvlvYVyvYlyVVvVYllylylVYyvlvVlVvllVVyYyVLVyVyVyVyyyyALALALALArrywwqtv                           .Tv ..,.,.,.,.,.........,.,.,...,.......,.,.,.....,.,
...............................,.....................,,,  .8B8B888B8B8B8BeB8B88888B8B8B8B8B8B8B888B8Beh8B8B888BhvV;vemGe9BCtyyvvvlyYvhhHtOB8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8BM;;;V::; .. ..  . .   . ...   vllvv;;;;:;;;;;;;:,.. .,;;;;;;;,      ,     .;kAVvyAyvlvvvbB8B8DCAv;vvyVryyvvvlyAyyyVvLrMmbv B8g4j8r    rB8eeg6Z@ppcjt2qBHvLylVvyyVvYvVlyVlvyyyYyVylyVVYylyyyvVVyYYvVYlvYvyYVYVYyVyVyVyyyyrrALryAyryAL4@pl.                           ,M; ..........................................,......,
................................................, ..:..  ;8B888B8B8B8e8B8B888B88888B8B8B888B8B8B8B8H8;yZ8B#B8B8B8cB;,,;kAe8B8B8B8B8B8B8B8B8B8B8B8BeZg@8B8B8B8B8B8B8B8B8B888B8B8B888B8B8B8B888B8B8B8B8B88888B888B8B8B4l;.r,vv...,;.      .   . . .,llvvvvv;vvvv;;.    .;vv,, ;.    ...    .UcrlV;;yXMyvLTm#OGe90cmUrvVYllLyryyvvvVyrLwThX#bL   r88DOBv     ;B8geBgDZ0bccpqDOvLyrVvlyvlvYlvvyVVlyyLVVYyyylVyyYYYllylYvvYVvlvVvYlyYyYyYyVryLyryyyryAyrrrw0#v                             ;Hv . ............................................,.. .
................................................ ..,..  v8B8B8B88B98B8B8B8B8B8B8B8B8B8B8B8B888B8B8BYCyrO,,B8B8 gB8B8B8;.:.v;C8B8B8VV4OB8B888B8Bjr#Ujt888B8B8B888B8B888B8B8B8B8B8B8B8B8B8B8B8B888B88888B888B8B888B8B8BVv,;v,v:  :,. . .   . . . ...vrlvvvYYvv;,   :;v;v;: .     ,.;:.  ;yVVvVvlMNTMALyHbDCjkHCeOpHm4ryryrLAwAyryyyAAhm#cUv.    .8e888:  .   .Z8eB6eGO2jtjtC6UvAYYvlVyYyYYlylVlVyrVrVLyryyVyVllVvVlVllvvlYvvvVvlYyVllLyLYLyryyyLyLrAyNCOM                              :k; . ,.................................................
....... .....................................   .      v8B8BBB8B8B888B8B8B8B888B8B8B8B8B8B8B888B8B8B8B8b;VtHeBX 8BBB8B8Bq ;: , ;L..   ,NA;;;vLHN2e8B8B88888B88888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B888B8B8B8B8B8B8BTvv;::r,.: ,,.,.... ... ... :lhAhyv:.   ,;Vv;;,..         ,:,.YADcyvAMhrMAwNkHckHUmHmUkjq0pcjkwrMwHHkrkbjX2cHHM;.        A8#GB8;  .    28DO9gOCp2mCj9bvyylyYVVyyVvyYVlVlyVLVyVLyryyVVYVVyvlllvvvlllvYvvvYlyVlYyVyyryryrLrrAmGer                               ;V. . ............................. ....... ... ....... .
.................. ... ..... ......... ... ...   ..;;VG8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B888B888B8. B,.v,v,rBM.8;8B8ecOBBU ;;  ,     .      XLyeB8B888B88888B888B8B8B888B8B8B8B8B8B888B8B8B8B888B8B8B8B8B8B8B8B8B8C88Vvv; ;l;. :,,,:,;,..,.,.,.,.vAV;.   .;vvv;...          ,;vLywArAyyH4wrMAU4hrArAMwhkmcwyr4H2COODZZZ90D0bUUrv:.     .    ;CBcANBG   ,    C8DD@O##pbttZOVAAVVylylyVylVYYlyVVYLyyyrLrYVVVlYyVvVvvvVvllvvvvllVVyVVVryyyryryTwqGkv                                vl. ............................................... . ....
. ......... . . ......... . . ..............  .vUtBB8B8B8B8B8B8B#bBjl;;vCB8B888B8B8B8B8B888B888B8B8v    8B8Z..8B8B6vGB8:;pD9e89A4mU;.        .;,  ,B8B8B8B8B888B8B8B8B8B888B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B888B8B8B8eyly .;y:,.,.,,,,,.,,,,:,;,:,.    .;vv,.           ,;vlvlAAlvMcthhAMN4XwLryyyrrrrAAmHmmcmhv.;lYyyylvv;.        . ...  88BGNUZ0;   ...   U8Z0pOpCcj20GVlkLyYVlVvvlllVvlVllYlyYVlLVYYyYvlyYVllvYlvvlvvvYYyVyVyyrVryrLMMpgDr                                 ,N;   .... ....................... . ............... . . . .
.... . . ... ......... . ....... ... ...   .;vAtwrM8eLtcDBvv;. ..;  ,vv888B8B8B8B8B8B888B8B8B8B8B8B6 vU8q    ve8B8B8B8BZ;vlA4mAme8B8Hlv;;Yv. ,;v    B8B8B8B8B8B8B8B8B8B8B8B8B8B888B88888B8B8B8B8B8B8B888B8B8B8B888B8B8e8BykV,;v;;;;,,:,:,;:;;;:,.,    .;v;.            :;vlwrlVLywc0thywhTAALrVryyyryryAMXHtkwv:                   ..... .  ,89XUCe8H  . ...    w80jb#mtttC8rvXTVVvvvvvVVVlVvvvyvvvvvlVylVVyllvlYyVVvlvlvVllvYYyyrrrVrrAkCG6h;                                  vA:  ............... ................... . . . . ..... ......
. ..... ........... . ........... ... . ,;yX4yy, ;XZl;VYHy, . ,    v:b888B888B888B8B8B8B888B888B8B8B8  B8@vB8v B; 8B8B8Bc;;vv;;vrvvjM .vmq8l:  v:   ;B8B8B8B888B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B888B8B888B8B8B8B82jc;,V;::v;;:;;v;;:,.. ...;vvv,            ;,vUUhrALvhbjHkTAhXAMrryryLyryyVyyhMUUwv,     . .       ..............   UBOl 2BH   .....    UgtcjjcjH26cvUAyyllyvvVyYvvyvVVv;;vl;vYylyyrlYYyVrLyVVVylyvllyyrrArhXcO#r;                                    Ay   ..... ..... ... ... ....... ... . . . . ......... . . . .
.. . . . ... . ..... ... . ....... . . ..;;, . ;TN:,vvv,;vvv;;y8g:e8lOOeBm;Y08Zc98B8B8B8B8B8B8B8G8B8;  ;y29A; ;c LB    XBv.yYVvvvyLyhy    ybB8B8,    8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B9Ueb;vl;;;Vvv;;:,.   .v;,.;.    .;v,   ,..rpUyyVrVTXtkkTANhLAyLLrVryyVyyyyArXTwv:     . . ........... . ... . ...  .8B0  U8O.  .... .    wOUjtckmt@OvATyAvyyVlVvlvvvlYl;v;vlvvvlVYrVYvyYVLTyyYyYyyYvyLAwkH2OmA;                                     ly:   . . . ... ..... . ..... ..... . . . . . . . . . . . . ..  
.   . . . . . . . . . . . ....... . . .      .;yv  .;Vpv;;vvv:.v6G6N8B8Bc  ;;,  ,LmO8B8B8B8B8B8BBB8B888V;   vc888B     .20: vyl:vvLyAk;      #8B     B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B88888B8B8B#pOhvlMvvv;,. . ..;;v,  .   ,;v:   .:vT4jAryrrcHUMUUAwMLAyLyryryyVyYyyLLwXXL;     . . .   . ... . . . . . . . .   MBeV  Ue8:  ..... .    hjtCckUjp8lAAMyyyrry;VllvvLV;vvvvvlYvlYyVyvvvLYyArYVVAAUmqZDCcUUY                                       ;Tv    . ... ... ... ..... . . . ..... . . . . . . . . . . . . . .
 .       . . . . . . . . . . . . .   . ...  :vv.  .UgkV4v:vjB8B8B8BMB8UV:      vvl68B80rk8B8B8B88BB8B88        vBv      X8t. ;:vvv;vvwcj,      j: . .8B8B888B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B888B8B888B8B8B8B8B8B6p8BhAv.  ...,;vv   ....,,:     vVrrwyrktUXNHHUMXwMLrrALryryLVrVyyrywhkrv.      . . . . . . . . . . . . . . .    Z8O;  lB8v   ... . .   .pHpCHkmjelykrrAryrVvvYVlvyvvvvvvvyVyvYYyyvvvVryXhkH0jtmXv;,.                                         lcy   . . . . . . . . . . . . ..... . . . . . . . . . . . . . . .   
.   . . . . . . . .     . . . . . .   .   .;v,  . ,rUvlkbC8B8B8BqD8mc2vvy.      :rB2H. ;qB8Bck8B8e8B8B.             .UB:hB8; ,;;;;::;vAt#kv         :B8B8B8B8B888B8B8B8B88888B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8BOyl.  .,:;v;,.  . ..;,:,  ,,VMhT4rrwHUk4kLMwMTXrAAArALryryLVLyyyMMkhV,        .     .         . .   . . .  .   ,86C   .ZBr    . . . .   ,ktCcUmm6XwUrLMLAryvvvrVvvlVyVyvYlVyArMNjccwX4HUAvv,                                               ;Ny,   . .   . . . . . . . . . .   . . . ... . . . . . . .     . . . .
                 . .   .                 ,;;   ,. ,kUvB8B8M;     ,;vvvvvvl;        ;VlvV:;B9;MB8C; tB8j. :AyMH4U.     .y ;8H.  ,;,,:,;vllUUCv         v8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B88888B8B8B8B8v      .,;         ...    ;kUkOMvvVwUXATyhMXNhMwAMrMrALryrVyyryTwHUM;        .           . .       .         .   ;B9k    99X   .     .     ;jbmmkkZ#MwrrrAANTyyHXjU4XjktjtccUHUUHUrv;,                                                    .YUA.    .         .   . . .   . . . . . .   . . . . . . . . .       .   
                  .                     ...   ,.  ;yU#8BY          .;ywv:lU;   .;   ;Vwrryy ,U8BC  :e6m88      .vAkV    ; pBT  ;, .;:.,,,vAHc:         v8B8B8B8B8B8B8B8B8888888B8B8B8B8B8B8B888B8B888B8B8B8B8B8B8B8B8BG;     .;::      .. ..,..   yctLAwyvlyUwTAkcHhkXUXUTMrMArrArArTTXUmHwv,                                              vBDA    kDU    .     .     rCckHU#ep@G9qZeOqZc4UAV;;;:.                                                                .yUl,       .                     . . . .       . . .                      
 .     . .   . .                             .   ,:y2N.              ,vyvvXy;   .   vyLLlVyvVZUv    .8B8B;         mBv  vL 9h,    ,;,    v.         .:;UB8B8B88888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B888B8B8B8BY     ,:;v;,,;; .,,.. ; .lYykc4MkMrXCcccjtjHckU4HwXwXNTAwwwXUwmmjjc4A;                        .   . .                   UG@;    rbp                 HpthcC8wv;:.;.                                                                           vMkY       .                     .           . .       .           . .       
                                             .  ,, vl                 .;YAjMmV  ,,. ;YrYlvvlwCh  v   U8m H           @: mh v88AvktbLrbBB8B       ;kh: r8898BqB8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B888B8B8B8888l     ,...... ..;       .:YlyXXkMAkhXXmhkmpjtttHUXHkHX4UU4UktmpbpH4Vv,                                                     02C:    v2q.         .      ;jbmUZB                                                                               ,lMM;                                                                           
                                           .   .;.                     .;vt8Zjv        .vvvlYlL; y: vvB8 :BM,vUchAy   .  m  68XO0jB@46@yAGl       y    vkc4mc8B8B8B8B888B8B88888B8B888B8B8B8B8B8B8B8B8B8B888h:    .;,.     ,.         ;VwryXVAlyAALhwhATwHctHtcjmmkHkkUHHtjpmkAl:                                                          eUq     vmO                  v@jXZ8                                                                             vAmv.                                           .   .                             
                                               .;                      .:,;ACNA:        vyYlyVLy ;v v;yBy 8B8,:vlrqtLv    b,v;    .   ;YLC8v,vMV;vj     8BMe8B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8v.      .,.   ,:..     .,rLvM6DbAyyAVrUhyTUmNhhNNc2jmcmm4kUkkcmpkrv,                                                              Omc     vUC,                  XZkZB:                                                                        .vwhl;                                                                               .
                                              ..                       ..;:vvy.::: . .,vrrlVvyAT .w ,v;8BvL B9  ., .;8B    N,T;      .NNUcBC.lAUj9j:    Y8cHyrm8BgB8B8B8B8B8B8B8B888B8B888B8B8B8B8888Z;              :.    ..,;VvkXwCC4kMMUccHUkkkXUNUXXw4NttjcHUHkkmqjhv                                                                  eHc     vX@,                  ;gjt8;                                                                    .;VATv:                                                                                   
                                                                        .,;;vv. . .,.vcXhVrrYyNwv;4,.YvB8c   VU vv;:  mO;  A;:4V  ..  ,.vVTv.  ;AcMl     vLvVvlrVN8B8B8B8B8B8B8B8B888B8B888B8B8886v    ..       ,;;..   .;v;VvvyUhAyVrXTUwwN4XUwUhUhUwwAwrwkckcmHt#ZCY.                                                                    etH     vwc,                   UOc8,                                                                 ;YXwY;.                                                                                      
                                           .                             ..;;ykU;.   ;vMrAyrLrwcv vy,LVj8q    veBwMXr, ;B; ,BOBM   . .   .,. .. vwTr:  . .rvyvlhM8B868B8B8B8B8B8B8B8B8B8B8B8B89v   ..       .;::,,   .:;;ywMAVlAYlrctUAX4kXTwXMUhUNhMUwhh4NHHjpD6eUv                                                                       GUm     vwt.                   ,8j8Y                                                            .:vVrVv,                                                                                          
                                          .                               .,;vyqO;,.  .AhyVYyyyyY:YH;:LlBBv :A8;Z:,#8.  v  c8B8H.   . . .:;;,   .rmyv.:  ,;vv;VU69yt8B8B8B8B8B8B8B8B8B88888Y,   .       ,...;       ;rkYVTHrylyXcjrB8gAlwwNMhThTUN4hUUHHtc2OeBgjV                                                                          @Ut     lHb:                    wBBH                                                        .;vLyY;.                                                                                              
                                                                   .       .,;;VUh;wUYVXTyLylMywA,vTYlvvZ8v .A#vXD:;0H:. :Z8B8BB;  ......;:v. ,  lUNv:  : ,V;vvYv;C888B8B8B8B8B8B8B8B8Bq;   ..      .:   ..   .:vv;vyyvvvv;;vrc8B8yL882YVALhwUwTwkUmmjpO9DpUV;                                                                             mhC     yhC,                     G8y                                                   .;vyyVv;.                                                                                                  
                                                                        .   .:;;lAXM4HTAUhhrAl;vv.;;;rTY8By  qr,vcqvvZUvlB8B8B88y..,. ,::.:;;,    AAl; :  .:v;;vve8B88888B8B8B8B8B8Bl   ...      .,..       ,;vyyv;vVvv;Mc@B8BOB88#XyZ8trVAwkkk4ct@#bUA;.                                                                                  Cmj.    VUb                      vBX                                             ,:;vyVyv;,                                                                                                       
                                                                             .,vvYywLATvYY:;vvvyy;yAvvv;X8e  jr,VlD;:vy,;8B8B8B8h;,;;,.::;;;;  . .Ykv;     ,;;;;vUA9B8B8B8B88888D;    .:     .:, .  ..   ;vyrvvlVLYvvXq8B8B8B80jB8jBMy0BphXHtqCqUMv;                                                                                       jHb.    ykO                       DB                                       ,;;vVlvv;:.                                                                                                            
                                                                              ..;vVyhV; .;lrylvvvvh;;yY;tB8l :8l;V8lyv, vB8B8B8Bv:. ,,. ,,..;;,   ,2y:yv;   :;:, ;8B8B8B8B8B8H,   ,  .    :vv       ;VMYYkLvrYrTXrCB8B8B8B8B8BtrOBC#OMcB8qpmXy;                                                                                            kcj;    TmH                       :8v...                         ..:;vvlYylv;;.                                                                                                                   
                                      ..                               .     .  .;vyrv;ylv;vvvrw;ww;ycVYm88m  :y,vAvly, H8B8B888vv;.     . ;;.     ;yvmk.:,  ;,:v8B@r8B8BOl,   ,...  . .:;,,: .;:;vlvYlUrvvvvvr#B88888B8B888B8gZc8BeBprkyv.                                                                                                UUC;    H#v                        vjAyvYlvvYvvvv;v;vvlvvvllVvvvlvv;;,.                                                                                                                           
                                      .                           . ..,,:,:,,.,,,,;ywb0y;;lryhH;vr;;XHLvcB8DH;    ,,,  ;8B8B8BevvAVv. . .. ;2CA    .kM;  ;:.  ;vZBD.vUA.      .    ,;v,..   ,v;;vAAyrl;vv;AteB8B8B8B8B8B8B888B8OcCH;:                                                                                                      hHk;   ;jb                           ,.;;;;vvv;;;;;;::,,.                                                                                                                                         
                                     ,.                  . . .........             .:vrUyy2cVkyvkv.MwAyly8B8B8Gl;..,:VB888B88h  :HYv...,.,  :Yl     .BgNv .:  ,beql:,.     .     :;,     ;:vvyrvvMNAVlvyp8B8B8B88888B8B8B8888h;,                                                                                                           mkm;   MGM                                                                                                                                                                                        
                                    :....       . . ......:,;,,.                          :;YyvXjvcBqcUXb68B8B8BUB#L6;vB888880   ;l;, ,,..,..,;;     .NBc,. . .VY. ,           ..     .;wXTVvVAlyrvlUO8B8B8B888B8B888B:;8pv 8v    ;                                                                                                        tXm:  :OC;                                                                                                                                                                                        
                                   .,. ... . ..,,,,::;:;:;,.                                      :vwqt4O8B8B8BvB8B8B8;.880yN888Bl;;,  . . ,:;v. ,,      :. ;;:,.            ..    ,.vvvvLvVvvvMtZ88B8B8B8B8B8B8e@trm8  ;v:,Z8:   ..                                                                                                       OHC.  XjU                                                                                                                                                                                         
                                   ,,.....,,;:;:;,:,,.,.                                                vB8B88vm8B2B8B8:8t,w;;e88Bv:;.   ..,;;;;;.    . .;;;;.      .      .    ,vrLyVLyvVyvyh#B8B8B888B888B8B8y;HcV;rv;;: ;rBXv   .                                                                                                       ZUp  .qcv                                                                                                                                                                                         
                                  .;.,.,,:..                                                               AX,,:.   vY:rM8B8BZ;;B8U.N8b; . ,.,;,     ;;;:.       :;.  ..     ,UcMyll;vyNvw28B8B86;;8B888B888888. ;;Uv..,    .vvl.                                                                                                          GTt  4cc                                                                                                                                                                                          
                                  :::....                                                                                 ;y8B8lr88.YBe;  ....;,  .:;,.      .,.      ,. :vvrrrvV;vyNrAj8B8B8B8BB.;e8B8B8B8B8#y: : r:.:.      :..                                                                                                         .OUX  qDv                                                                                                                                                                                          
                                  ::..                                                                                       .4T68BV..  .... ,:;:;:,       ;;.  .,,  :vNrhrvyV,;lrmGB8B8B88888B8B;v#B88888B8Bl .   l;  .                                                                                                                  :9UM l@t                                                                                                                                                                                           
                                 .;,.           .                                                                               UB8Lv,,.,:::;,.        .;;v,  ,.. :lYAyvY;vyHUm#G9888B8B8B88888B8lA#8B8t888B8       :                                                                                                                     vtkv XOY                                                                                                                                                                                           
                                 ;,...       . ..                                              .                                  ;GHv;;;;,.       ..;;;,.  ..;lvlArVV;;lT98BeB8B0B8B888B8B8B888BhyqB888cpN,                                                                                                                              vcX; CD.                                                                                                                                                                                           
                                .:,......   . . .                                           ..                                     wA;.,:.   ..,,,;,,;.  ,:.::vLXrv;YrAe8gCyvvmetBO88B8B8B8B8B8B8ryk8B8Bj         .                                                                                                                       MXH :@c                                                                                                                                                                                            
                                :;........ ... .   .                                      .,,.                             .         .,,   ...:;,  ,  ;;;ryrrv;vvLAGB8e4v;:;;vv;;Hy8BDB8B8B8B8B8B;vvB8B8j8                                                                                                                                cNc vZM                                                                                                                                                                                            
                                ;,,......   . . . . .                               .    .,;,                              .:,       .   ..        ;vvVyrVAvv;YUGBOHV;:,;;vvvvvvv:yB8e8B8B8B8B8B8,vv8B8yyee                                                                                                                               tht VBV                                                                                                                                                                                            
                               ;:,.,.,...  ....... . . .                   .    .........:;;                     . ,.        ;:        ,.     ..:vVml::;,,.vrjB8A;,..;:vv;vvvvvv;v#BB8B8B8B8B8B8Bv,VB8BvVhO8;                                                                                                                            ;UHA V8v                                                                                                                                                                                            
                               ;;,,.,.,.. ,.,.,.,.... ... . . . .     . . . ....,.,.,.,,;vv.  . .   . . . . ..;,. .:v;.       ;;,           .;rkrlAv;;vvl4OB8BC:..,,:;;vvvv;v;;vyVw@jj8B8B8B8B8BcvvB8BUvv,4B8w                                                                                                                           Ambv yBl                                                                                                                                                                                            
                              .;,,.,,,.....,,:.,,,.,.. ................,.,.,.,,,,,,,,;;;vy; ...,.. . . ... ..    .  ,v;,       vv,      .;vTLyvAllvyhU29B8rv;,  . .v:,,,;;;v;;vNvVU9wYj8B8B8B8B8v;,8B8v;vvBy#8O                                                                                                                          UUG  v8r                                                                                                                                                                                            
                              ;;:.,.,,::,.,,:::,,,,.,.,...,...,.,...,.,.,,,.,,:,:,;:;;;vVv, ,.,.,.. . .....:;,. .    .v;;.     .vv;      rm;,:vrVvyCO0OB8b:    ;vr2OHrr;,.::;yhVkcbctZXy888B8B8U  vB88Uy8e6Ojg8B                                                                                                                        ;kUC  vBU                                                                                                                                                                                            
                              ;;,,.:,,,:,::;:;,:::,:,,,,,:,,.,.,,,,,,:,:,:,:,::;;;;;;vvlyv.,:,,:,:,,.,,;,..:vVv;..    .vv;..    ,lYv,.    lv;rZ8B888B8B8U:  ;YrtqDcHUmp@X:.vyrAm4XkAUrVvB8B888B;  B8BB,;vv,lHUBe8,                                                                                                                      ymcy  .8t                                                                                                                                                                                            
                             .;:;,::::;:;;;:;;;:;;;:;,;,::;::,:,::;::,::;:;;;;;;;;v;vvvlrv:,;;;vvvv;;:.  .   ;Vv;..     ;vv,.    :ylv;;;. :D8BU;,VOOprV:  .L99kmbXrAkHrk89By:v4qrVkZj;vc888B8BG  ,8B8:;vy9XX86vvGB.                                                                                                                    .tyj    BD                                                                                                                                                                                            
                             ,v;:;:;:;;;;;;v;v;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;vvvvvvlvVlrAyv;;;;vvvlYvv;;,:.   ,vv;..     vVv;:.. .vyvv;;:  Npv.;,,:      .yOyrctkmUqg8g8g8B..CCHlAkv.vlvB8B8B8v  ;B8;;;Hy2Blvv.;v8B                                                                                                                    vkUk    89                                                                                                                                                                                            
                             .l;;;;;;;;;v;vvvvv;v;v;v;;;;;;;v;;;;;;;v;;;vvvvvvlvVlVVryAkO0Mllv;;;;vvvvlvv;;,.   vvv:,    .yrY;v;:.vyVvv;;, ;w;hcbHpjM;vyU@gcB9be8gqctOB@lvyyALAHVLk:vq;.vB8B8Bv;;h88; vbyAv.: .AAY8B                                                                                                                   mrO;   ,BO                                                                                                                                                                                            
                              vv;;;v;vvvvvvvvv;vvvvvvvvvvvvvvvvvvvvvvvvvvlvlvVYyVLyLLMh2B8B8B8Bbrv;;,;;;vv;;,.  .vlv:;,,..vMylvv;,;TAlvv;v ;rvXvvAyttXMOe8B8BBOkv;;.,w. ;4AmhY;VTrVtBy.;.yB8B8,v;vB8v,H8c;;vrCBy  A8B                                                                                                                 vUTt    Mbt                                                                                                                                                                                            
                              vvv;v;vvvvvvvvvvvvv;vvvvvvvvvvvvvvvvlvvvllVlyyLyrLALAwH08B88brrAZB8B8Bcv;.:;v;;,.  .vlvvvvv:;yMvlvv;;yUVlvvv;;vlwv;lYVMMHVvvVHlvyv .,  ;y  lmHy;vlvvHbv:;;:.ZB8B ,;v8B, ;B;:;Nwv8Zlmv.#8v                                                                                                               jyb.   .9M2v                                                                                                                                                                                           
                              .Vvv;vvvvvvvvvvlvvvvvvvvvvvvvvvvvvvYlyYyVyVyyryrAAM2D886my;     .:vU68B8B8v,,;;;;: .;VlllYv;:yTyvlvv;ykwVVyVvvwTwYhVwUrAywyvvqVrAcB8tl ,k:  ;. :wryv;,.:::.,08B8  ,;B8  e8.,;;. rGZ8, ;8Bp                                                                                                             LVNM    V6AUU                                                                                                                                                                                           
                               ;lvvvvvvvvvvvvvvvvvvvvvvvvvvvvvVlVlyYyVyVrrMrwUt#G#tY:         .   :vcO8B8q:.;vvvv::lwYlvv;;vwrlvvvvVjhryrYvrjyTTvYAkyvwywlALAry:XB88c;   vMvv;yvV .,;;;:,A@m88G   88lvy@q@BC4b0 .6v,,v8B8.                                                                                                           MVU,     ,                                                                                                                                                                                              
                                vVvvvvvvvvvlvvvlvlvvvvvvvvvYvYvYlYYVYyyrrhTHbbUAv:              .....,VZB88v.;vlvv;vArvYvv;vrAvllvvMmwyryV :O;vkhcMAyrrTlrHTMrvLUmy;B8;    vcryHl,,.:;;,.XV.c8Bv  h8B8  :vrmH:  Z8;v VB8Z8B:                                                                                                        XVvc                                                                                                                                                                                                     
                                 lyvvvvvlvlvlvlvlvlvYvlvlvlvYvllYlYYryAAkUUAv,.             ,;.    .;vjg8B88v,vvvvv;YrVvVv;;rryVyvYTkyyyy; :89Y;vvV4llwrNkyMXyXcA v:vB8y,MAAArtrCOby;.,;.., ,B8B; ,B88t  ;v,.vt9j:; ;B8B,w8Bm.                                                                                                    ;BU,;8,                                                                                                                                                                                                    
                                  lMvYvYYVvYlYvlvlvlvlvvvlvlYVYyYyyrAUwAv;.. .           ,:;   .;vvhHTVUC#Z88V;vlvv;;yyVvl;vyTrryVrwyyyyvvvv;llyVvlGtvyh42rVv:YMv;yV :98Bk,;y2cmmcm9m:.;,:.. vB8B; v8888v ..VGw;:;,lZwB8  B8B86L                                                                                                    .TGp                                                                                                                                                                                                     
                                   lUrylyYVlyYylVlVvVvYvlvYlVYyVrAwrrv,   ...         .,;.   :lYvvvvLllH#qjG8#;;YvlvvlrVyvvvrAryyLAVvlLyMAl:,,;;v;yB8eYvv;YyV:.vMvT4vv;#8B ;XrvMUCOBg:.,;::,. LB8B, .B888v.vO8tkXB8v ;8B  8B888B8M                                                                                                                                                                                                                                                                                                           
                                    :kcTyyVyYyYyyylylVlylVvYVAwhAv:,....                  .;vrLYvryyyVU02Cc@Bv ;lrlvvryVVl;lv;;;,; ,HMlLv;:;;;,;, .vTCmylryytH;Vpcmkyyvvw8Bry; ,k8rA;.,:,;;:.,V888B.  G8B8B8  kM,;   8B6  N8B8B8B8eT                                                                                                                                                                                                                                                                                                         
                                      VjD0jUwAMLLyLVyVylrrAMXyv:...,,         ,.      :;vvAALyUU4MAt0@O9O0Ze8H ,VVryv;;lX4CB8B9;   y8qv;;;;;;::.,lkUjB8Hlvv;;vtBeDjL;;mv;B89;  ;8k   ,;::,::,. .tB8B. .U8B8B4       ,B89  AB8B8B8B4O8Y                                                                                                                                                                                                                                                                                                       
                                         vk#OgOD0bcUM4Xk4hyv:,  ...      .,;,     .;;vYyyrryVNAwH2X9eB6OOG08B8B;vAy;OBYvkAUqG8B8B8B8Br;;;;;;;;,.vee0DGv.,;;; vZ8V;vAv;UU;ll: :.,beH,.;:;,;:;::..;GB88;  ,8B888BO    r8BO  v8B8B8B8B9#BBU.                                                                                                                                                                                                                                                                                                    
                                              ,;vVMAUUhYv.  ..,..     ,,:.    .,;;wrLrMAhyVvyXjGDHHGGOeeOp2B8B8evvv,HB8;.,,   8B8B8B8v;vvvv;;;:;ZetUb;  j88eBB8BD. :8Bc,.   .,.leBk.,:;::,::;,:..v8B8B; . AB8B8B8v: ,e88. ,B8B8B8B8B88mZeM                                                                                                                                                                                                                                                                                                   
                                                     ...   ,..    .:,:.   .;;vLArw4HATvvlAjBGeBGUC6GGeBZcAB8B8O8lvvvB888;v;;jOB8B8B8@;vl;;;;;::;YtB#jv .t8B8B888BUv6B8e; .;yy. ;;y;.,::;,:,,,,.,  ;8B8B;    V8B8888G UB8   8B8B8B8B8B888tCGr                                                                                                                                                                                                                                                                                                 
                                                   ,,    .,..  .,.      .;Yyv,vlLyryVLUteB80@GpXp8BOB6py8B8Bb.vBOvvv8B8B8rV6888B8B8Bh;lv;:;;;,:. .ymHt8mAvMq8B8B8B8B8B8qvvwvVXcq8N; .       ..,   ,v8B8Bv     v88B8w 88B;  k8B8B8B8B8B88886jDv                                                                                                                                                                                                                                                                                               
                                               ,;v,  ..,,;..,v:      :. .;lVvVyylvAm0#Z888Zp6@rCBGO90ObUt8B6;vvvk#BGB88vU9G8B8B8B8B8vVv;::::,,,:,,.,,;;l.  ;j8B8B88888BBpv..,VTM;    .;;;;:    .rZy y888B8v ,;,;B88. 8B8v.v,88B8B8888888B8B8BZm:                                                                                                                                                                                                                                                                                             
                                            :;;..   .,:,;:  ..,.:vtyv;lvlNUryyrXcmbB8BBG9cjckcOG8GGG9GZ48B8;UB8qq#t;;B88,,;.BB8B8B8g;vV;;;;:;;;:;:,,..,..;wcvvOBmv;v#GM;. ,,,      ,pB888B8BBjkpX;  .yBB8B8B; . ,B8BOB8B8 rH ;8B8B8B8B8B8B888B8Bm,                                                                                                                                                                                                                                                                                           
                                         ,,:.     .,     ..;;:,vvyv,YUcMyrvyH#e88eCtwtqZjCDjrtMA28BB6ON8B8v lM;vc8N; 8B8tv;:y888B8Bq:;vvv;:;;;:;:;:;:;:.;pjc  .. .y.:reY. ..  .vv;,cBe66, ;vrV;. ;;  .VGB8B8B  . YB8B8B886 vBv;HB8B8B888B8B8B8B8B8er                                                                                                                                                                                                                                                                                         
                                      ...       ...   ..;;vvAv;;vrcUjwXycCO98B8BBB8B8B8bX2cAcB8Cryteeb#88B8O8mL;v8kvlB8B8;yv,88B888Hvv;v;;;:;;;:;;;:;,,.;Hjh.   ,G8.  wBU. ,vH9Be0@@bqgr,.     vVlyyv .v088B8B; . ,q8B888B8v L8Mv:D88B8B8B8B8B8B8B8Bg8V                                                                                                                                                                                                                                                                                      
                                  ....         ... ,:;;vvVlyrNMkAyLUHmCjCqB890teeBe8B8Bkt8ZYrYAZBp;yZ888B:U8mtm88H;wY;88BVvl.hB8B8BYvv.:;;.::;;;:::;::..:jeccke8B8;   H8v ,OqeO8B8eGGGB884  .,yv.,.;yy  v9B8B8BH  . .8B8B8B8B;.XBt. O88888B8B8B888B;.yw8X.                                                                                                                                                                                                                                                                                   
                              ..,       .      .vlvvV;:;ylryyAt2bq6OqHllDrvHMjL;9B8eBe9VZBOveBrvAGBvyB8B;;A;;;:l8AVvv H88q;Yvv8B888.;;;,.,;:;;;:;:;:;;,.yB8B88Dt9:   L8h ,OB8B8O;yB9ebVv  v;YAL,,,;: ;X; ;jB8B8B8  ,  8B8B8B8B8; ;8B8vCB8B8B8B8B8B8   lvGBHv                                                                                                                                                                                                                                                                                 
                           .,,      .:.      .:vlVvVvvVUHUXjZeOC#jv:vv;8Bj;kB8BvkH26B8UvO8crZC@BM;UBB8BCO8BG6Y.:vO8rv;B8B#.vv,O8B88,:8BH.:;v;::;;v;;,,:..vACDV    ;rH8c. .;ywXv   ..,;  ;y;  ; ..,:v: :Uv:,wB8B8B8v .  C88B888B8Bw ;B8B8B8Bbl88v.86.:; vBvv8Cv                                                                                                                                                                                                                                                                               
                        ;;, ..   .. .     .,;;v;;;VATk9#pCGB8lY ;;;v  DB8;jB8v;C8Z.MBehV8BkTMMeOpv:B8B8eBvv;p8e8@vHBv;8B8B:;y;8B8BcvYBBv, ..,,:,,,;,:;:..      ..,vvv .  ,        .;vvYv;:     ::,;,v;:;AvVB8B8v6B8t .; 8B8B888B8B8v UBB8B, 8O .YV:;. ;p8.;  p4                                                                                                                                                                                                                                                                              
                    vv,:,.             ..;vv;vvVLMHjH#9CXNMAv L; vB8B;v8B8B8UhB8BpkeegUTBBvTBOCqc9B888;vhcOv:8BAvvc8;.;8B8A;v;r888yv;; .,,,,:;,:,,.,,:.,:: ,.,,:,,.    . .., ,  ;Vv:  .l;   :lv;;..;:;,,,;vvO886Y8B8e;.  888B8B888B888B89  8y ;8;.;;v8.cBv  .tBr                                                                                                                                                                                                                                                                             
                   ;8Z;          .;;,.:vrLAYlw@ODHDBetXVvv:..;yU  eB8BYYvMbG86V;;kgeOepv#BTYgmv4eB8B8lVAlv8V;BHvvclX8l.B8B8;v:.B88;:;;:,;;v;::::;.  . ,,....,,;,..,.. .;vvUAAMr;v:      v;.   ;;;,.., ,;rvVVyllB8B8B8X . y8B888B8B8B8B8  vO,  8l rV;8OvT; vZcvU9c                                                                                                                                                                                                                                                                            
                   ,B8V,     ...,;vvYLLLvrTHb@9#GOyyv;  ;  ,lh ;bv  vNr0Y;YeeDj8BvYg9B@;k8cvrk:HB8Bc:8ZL;;Xkk#etH8Hvmct8B8B;Yv 8B2,vvv;,:;;;:..:vjkM:,  ;: ..,::,,.. ;yy.      Av       vllV;;y;..... y4;v;;  ;8B8B8B8.:  B888B8B8Bkv8. y8, :8B ;kvVAOv ;NV;  .vGp                                                                                                                                                                                                                                                                           
                    XBGHyv. ,;;:;vyA4HtMhHj686LAA: .v   ;  vX,  .vNv.   vvAB8B86Zy4Ct0H:vbBrvw8B8B8Zyh89AlV;vvvVky;vAv e8B8mvv;OVv;::,. ..;,,;v; ,kB84v ,tc;  ..;, .;Mv        ;;   ;,  v. ;V;. . ..  y       BB8B8B88k,. 8B888B8B  j. v8. y8V8.X.rvv  vHvv; ;8TrB:                                                                                                                                                                                                                                                                          
                     0B6Cjrv:;vyYyAHUcOOB8#L   v,,, ,. .. ,w;.;;:   .vvv;:;yvv,;;VjCyvlA:vZp:bB8B8BOvYkq8Cyl;vllvvlyvv ;B8BOvvvLlv;, VUv.....yrvv, .ADCA.;ekv;  .:;Tv;         ;;   .v;vA,           yY. .   eB8B8B8888t; ;8B8B88  BM y8; .8H :er Zv:Mhwv;    @OjOy                                                                                                                                                                                                                                                                          
                      2BqMbkyVMcjcbG8B8jY    ,X,;v   : .,;M;.;v;A.,;;   :vv;v;;;OBOtXUGBv.L6O88OZB8rvvrv98yvCClVHAlrtvyBOB80,;rlv;C6;#8Bv .;YA; ,A:  v9Or vqcrL, ,MLvvvv,       yv    .v:      ;v;,,vh. .   O886e6Be888Bb,vB8B8U  8N v8:  8B; vj.A6 vB8.,.yB8BkUbBv                                                                                                                                                                                                                                                                          
                       VBZXHHkm#O8B#v       vA.:Vv  ..vv;V,  , :lykDVYUv ;:.,vlUv,lvyLU0qvr8B8v ,8B8;:vy;C08B8GLLwX8B6e; 8BZ vyv;qB8ByB8.vw;;,  ;kry  ,UXA vBZpr  TY. .vyyy: :vU;;;           :c;vlvv, .  .8BBOG996BG8B8B8B888v  BM v8: Y8:8, Aty8.;OOB   v8BcVv0T                                                                                                                                                                                                                                                                           
                        :B8BBZeOh;         MM;,v,   ,:;;v, ;v;yrV,vcv;v;vY;:vHUv;vv,vlTceB8B8k;;;B8882kvv;TBV:rhypGv;v; vB8U;yVvvAcb; hv;v,   .;Y;,YX, vUpV VTyyv.Thrv  ,.;;hyv .,;ywLl;.    yp.          BBeGO#OOODGGBB8B8B89,v8V ,8; .8B,  kyevvl;8V2 ;B8v v8t8:                                                                                                                                                                                                                                                                           
                          ;HGT.           M;;;v.   , .vv lA,  v,  :Uh ;vv::H2ylvvvvLlll@B8B8#:yyv8B8BlO8l:.8Bv:VZk;ll:y8B8BvVylvv   :l  vyH4v,,:,.,,;c, hky. vykU .CcU: ,;     . .,,,;:LrLVYYl:  . . .  .jBCpO9OZODOOG9Be8B888B8V v8v .8B8  B8;MbNBOBekye8; ;rB8C                                                                                                                                                                                                                                                                            
                                         yy :;Y;:.  :rv  ,v   Vyv;,.Av;, :cy.;vvVLMyv;;88B888vV;;B8Bv vB9Hemtv;T8BZYrv8B8Bm:yAv:, :UY .ykvr;.,;:,,;:;h; ;bH; MTr2,.cZ0r L9v       ... ..,;;     ..,,,  .UOmXcc@O9DO#ODGg9eBB8B8B0q8; ,8B8B  G,8GttBc9mZB;y,v8MBB                                                                                                                                                                                                                                                                             
                                       .:cyv;vV:   vv, , .;l,..Y    ypv :Z; ;vvyvvv;;y88v6B8B,vv,8B8M;XLXe;vvGBM,UkVOmq8B8yrrV,. ;42;.yVvv;.,:,;:.,:my.. lby ybjkv 4N#M wD2; ;y;   ...         ,.,...  H0kAUXmtCqOZODDZ9DG9eB8B8B88OB8B8B4. O0U.Ze.vBtBcAD;lb9v;;                                                                                                                                                                                                                                                                            
                                        ;v;v,ly   Mr   ..,vw   ;v,v;;  vj; A6MrlvvLvmB8; ;8B8; .,B8B8;vYveYYDBCBvvYv; #B8HVrv..:w2OHMM: .,.,.., ,.VLv.,,  jC.,OmD; rmtv XZO: r8BA  .;,      ..:.,,.   r#UrMrMNUXmbZZO#ODOZ999GBe888B8B88e NB8kw.8j,AOrcrhO8eV;;;Vy                                                                                                                                                                                                                                                                           
                                      .vv;:;;yV::w;   ,;., rYvv;;v;,;.yt. wkvv;vv;;rt.  ;:V8B8 : vB8B8; :8OvyYv84Yyv.6B8Btrv;;vkBZ;;Xk;   ,.;,....Y: .,,  A8r MOt vCj6.;C9X   .:Nv   .   ,.  .       ybmyryArAMhNmcC#O#ODOD9OO9GOOGeB8B8B8BOk2B9jtB8;:mZc8bv;vvv;LA                                                                                                                                                                                                                                                                          
                                      vvvv;;;;,vhv.      :yr;,;v;,v;.;krY;lVA;;vyw99XV,v, A88B;;y;8B8Bc.vvkHvterV; B8B8AwB4vv;rN;;vry,vv;       ,UA.,.,. lm#X Ugr TbB; A6B; r; .tO;     mB#     ,   rCcyryryryrA4XHm2CD#OZO#OZO9GO9GD0O88B8BOB898B8vLB8k4VvvvvvvvvYy                                                                                                                                                                                                                                                                         
                                      ;vvvvv,,vvv;vrlTv   .   .vv..,,cwv:;.:v;.:yUbv O2..;;tB8B;v:A888B;;VVlv,.,: v8B8b;Ce9ml;;. ..,.. ;kl;;;.vUAr,..:.,;c .rvB0 ;B8v ;8Bv hBr :vcBN    ;w; .,...  A9mrryrLryrLALXNUkcj@#ODOD9O99e6eOeOZqeB8B8BrM8B8B8VYLAVvvvvvvvvLY                                                                                                                                                                                                                                                                        
                                    vvyv;v:v::; .;,;  ;vrlv,      , yX;     ,L4AvT: ;B9;rrvv8B8;:;;8888y;yNry;vjkG8B80Lj#bO69b4lv;:,,.vUbBm;HBD;  ..,,. Y: .;vL  Z8v v8e, yB8:    hj:     .   ..  rOjMwLryryryArAAwwUkccbbDOGO9OeGeGe6B6eOOD6e8e6B8B8wVyAyVvlvvvvvvvAv                                                                                                                                                                                                                                                                       
                                   AHlvvv,;;vk;veeg8v ,    :lAU. . vtv :,;:rB82; vy:.X; ;vl,j8Bj;v,B88B8vvvvv;B8B888OyCOb0bq@GeBe0l;,vO8B8: .HOV ..:...:  vy  lk88, v8j. L88Y     vU   . ..,...  yGjMNTMryyLyArrrMAThUkHHp09O99egeGgGeeee6O6OqmHhrVYvVrryVYYllvvvvvvvrv                                                                                                                                                                                                                                                                      
                                  ym;;;vyvv:vM; v .Ve;:;v,L;  ,ypXvHelvOb#eB8p,v..tb.v0cCl. eB8Vvv;B888By,.  G8B8B8,kZq2@2CpCCqqOGBbwVXHc:  Ve6;  .,. UeA,v  ;ryh .t8: ,CB8v  ..  tq.  .,...:   veqkXAhArrrLArArTMww44XUmm2bDD99e9G96G69GOZ0@mUTArALryyVyVVvlvlvvvvvvvVv                                                                                                                                                                                                                                                                     
                                 ;ty;vvVlvvvvwr: ;0OrtmMev;8B8X: :vU2y.rj8B#v   B8p8m8@YAB;;B8B;;l:888Bk;v;gB8B8B; mOeZC#ZbCbCCqC@#eBbvv   :jDe: .   ,C8A  :yl  ;DZM. vDBD,  ; ,.rZO2y...  .   ;@Om4NNAMAArArArArTh4hUTHcq0#C#ZO#OOGO9OOZZCCHUXXhTLrYVYyVVYYvlvvvvvvvvvly.                                                                                                                                                                                                                                                                   
                                 yv;r;vvM;v.Vk;,CB0.;jv,Bb vD8Z;vr: ;l,vD#Y ;,vMjB;  . ;:v8B8B8vvv,g8BB .v8B8B8B,.0GOZZ@D##qqpq0CC02ZG8BZL;X88G  .   yCcrv;,,   :,  ;Z8G;  rt; ..,  4Zv       .j9mcHUhUXXNUwhAwrwwwMUUjqZbjbCt2pqC#0qC@b22pm4hmXMyyVyVryyYVllvlvlvvvvvvvVy;                                                                                                                                                                                                                                                                  
                                vwlvV;vvVv; rr AXv ;eX vB8:  ;vk#krv,vvvlV;Yl:Myce,vtUy;,.w8B8B:;v:8B88: 9B8B8B:ye9OqDOGOOZ##D##CCj@C#Z9e898B8N;: .vm8ZOG8U  .:HGCB8BU;  vHZl      :V,   .   VBBqtmckmHmkkUUX4XNwUUtCOO@ctHtcjctttmpjbpjcckUkcUXryyAyyVyYyVylVlYvlvlvYvvvvy;                                                                                                                                                                                                                                                                 
                                rvvry;vyvl;.Tyv,;.;B8v.C8q.vUNjv;.vV;;vly;vOy;Z88A@BvM8C.;  8B8.vv,B888  B8B8e U8OOD9DOZODDOOZZZDCZZ#CZ#Z98B8Bc.yB8B8yvMq0v vV;,k6r:  ,rH06l      vMch;..;, ;B8Opp02C2CjtmcHmHctCZgeOpmXUktHmHHHtmkmcmtmcHmkmkTAryrrryrLrrArryyllvlvvvvvvvvVv                                                                                                                                                                                                                                                                
                               ;rHyMlvvrVy;;wMNhU;wgL;,;4vvjCy:.;VVlvYlOy;YCjvyhvlvGv ;8... 980  ; GB8B ;8B8B;C6Hj2ODGOO#Z#OOOZOOOD9#ZO8B8B8V     A8X4::Nv      vv:YVANXbM.  ;v:. .,VeOv   ;B8@OC0CCbCjbtcj22ZOBBBOjkmkcmmHmkmmmUkkkHmHHUmmcHUh4X4MNMXwwATrrlYvllVvlvvvvvvvvVv                                                                                                                                                                                                                                                               
                               2HY.lA;v;Vy,,XrrAXv ,vr4;vr#y  yMvvyrccO4  vD6Uv.  .A6.;B;  ;8B8   .B8B8G8B8ZA6bNHtCCOO696GG99OOZOOZ68B8BB;           H8B8l.vl;;TqmYvXyy;,. :jB;  .v  ,2v  ,B8O9OOO9ZZqZqDZGe88860kmHcktctmcmcHHkHHcUcmmkmkmkmUHUXUtmct2HhLYvvvlvYvlvlvvvvvvvvly                                                                                                                                                                                                                                                              
                               lwHVlr;:V:l;;XhyTANMrhmNtmk; vU;;MrLwlv, :NXvU8BByvMBpCBk, y8B8l   ;8B8B8B8pvthTUHtcCpDOGeB66G9G698B8Bm,                 lMhvv;  .v..:,,vv4@8B; ,HGB0;;v. ;B8eeeGeeOGOeB888B8B9pmkmcbjttjttccmmkmHtctcjtjjCjjcpctp@OeO@mMVvvvvYvYlVvlvlvvvvvvvvvL.                                                                                                                                                                                                                                                            
                                ,0q@DOvvl,;;UUAA4rAAwrMTV.;HA;LXyyc;  ;X4Y,vqTXe8BBw;:...,888B  ,.UB8B8B8BkHrwNUckctC0Z#G99DO9BB8B8l                      vjGk, vC8OGB8B8B8w,  t8Z. UH: AB8B8B8B888B8B8B8BeZCtjtC@Z0Z#0bqp02Cbqb02Ctbjb200@qZOeB8BGbMvvvvvlvllVlYllllvlvvvvvvvvvV;                                                                                                                                                                                                                                                           
                                    .4B88OBl;hUUrrrhrhh;.A4;yOhvvY;vvyVvm8V.VGvmB8B8Z;   ;B8BO ;,;B888B8BmyyMkkUkccjj0#O#9Oge8B8B;                           vO86v;;;:.;.;.   .Vr.  yH; vqB8B8B8B8B8B8eeeeGOOeBBgeeeG6O9D9ZODOZODOZOZDZOOGe8B8B9twvlvlYyYYvYvVlVvYvVvYvlvvvvvv;vvy;                                                                                                                                                                                                                                                          
                                         ,rckwwyTANXkc;.2M:vrl;;vvvvvVCbMkv .OXh8B8B8B8B8B888v.,.r8B8B8BjvyTUUHmcmtc20ZD9eBB8B8.                               ,#B8X;; :40V  .     ;8g.     .r8B88888B888B8B8B8B8B8BBee6egBeBee6egeB8B8B8B8BGjNyVlyyLVLVyYVYYlVlYvVvVvYvvvvvvvv;vvyv                                                                                                                                                                                                                                                         
                                              AHAyvyy; ;: vl;Yvv;lrm98k;H8Bv.UO;M8BOB8B8B8B8; ,  8B8B8BwvyNkUHUHHtjjbZ0OB8B8X                                     ;GB8B8ev         mZ             ,vZ8B8B8B8B8B888B8B888B8B8B8B8B8B88888eZcUTrywrryrrryLVyVVYVlVlVlVlYllvYvvvvvvvvvll                                                                                                                                                                                                                                                        


*/

/*
                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                         ;0U.:.;NUwr;                                                                                                                                                                       
                                                                                                                                                                                                                        hB298B8B88g8888r                                                                                                                                                                    
                                                                                                                                                                                                                       vB8G86OB8B8OeG8B8B;                                                                                                                                                                  
                                                                                                                                                                                                                  :  m8BOq0B8B8B8B8@Z68B8Bk                                                                                                                                                                 
                                                                                                                                                                                                              ;Be8B8B8ZcD8B8eOCeB8B888B8B8Bm                                                                                                                                                                
                                                                                                                                                                                                           :H88Ok9868eDB9GZqBB8B8B8B888B8B8Bc                                                                                                                                                               
                                                                                                                                                                                                         :2888CCB8BZC8DZU8B8B886Zge8B888B888BV                                                                                                                                                              
                                                                                                                                                                                                        cB8B8UjB8emcjkeB8B8B8B8B8B8B8B8B8B8B8B.                                                                                                                                                             
                                                                                                                                                                                                      :eB8BHMeB8Mthc88B8B8B8B8B8B8B8B8BeG6O9e8O                                                                                                                                                             
                                                                                                                                                                                                     A8B8By#8BOyAC8B8B888B8B8B8B8B8B8B8B8B8B8B888BC,                                                                                                                                                        
                                                                                                                                                                                                    U888CcZ8B@;HB8B8B8B8B8BBB888B8B888B8B8B88888B8B8Bw                                                                                                                                                      
                                                                                                                                                                                                   c8B8cXB8BhH8B8B8B8BBe8B8B8B8B8B8B8B888B8B8B8B8B888B8k                                                                                                                                                    
                                                                                                                                                                                                  c8B8Ht88wyZ888BOG68888B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B                                                                                                                                                   
                                                                                                                                                                                                 X8B8U6BbwtB8B8gGG8B8B888B888eeGBB8B8B8B888B8B8B8B8B8B8B88p                                                                                                                                                 
                                                                                                                                                                                                y8BBk86Dr@B8BO9BB8B888BB68B8B8B8B8B8B8e8B8B8BBe8B8B888B8B8Bc                                                                                                                                                
                                                                                                                                                                                               .8B89O9tC8B8eZG8B8B8eOe8O9B888eBB8Be88B8B8B8B8B8G88888B8B8B8Be;                                                                                                                                              
                                                                                                                                                                                               G88B9Uj8888OeB8B8e6OBB8Bee8B8B8B8B8B8B8B8B8B888B8e8B8G8B8B8B8B8B8Bm,                                                                                                                                         
                                                                                                                                                                                              HB82pH6B8B868B8B8e8B8Gee6O8B8B8B8B8B8B888B8B8B888B8B8B8B888B8B8B8B8B8B8j;                                                                                                                                     
                                                                                                                                                                                             ,B8Gr0BB8B888e##gB8BOq8eeB8B8B8B888B888B8B8B8B888B8B8B8B8B8B888B8B888B8B8B8B#v                                                                                                                                 
                                                                                                                                                                                             @8Zc@8B8B8889mZ8g8BDO8BeB8B8BeB8B8B8B8B8B8B888B8B8B8B888B888B8B888B8B8B8B8B888B8v                                                                                                                              
                                                                                                                                                                                            y8AO08B88888GBB8g8BCG8BBB8B8B8B888B8B8B8B888B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B88eB8B8v                                                                                                                           
                                                                                                                                                                                            8pZg8B8B8BeB8BBB8B@8868B8B8B8B8B88888B8B8B8B8B8B8B8B8B8B8B8B8B888B8B888B8B88888B8OeB8Bc                                                                                                                         
                                                                                                                                                                                           rB6B8B889Oe88e8B889B8gBB8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8Bg98B8e                                                                                                                       
                                                                                                                                                                                           98B8B8BC98B8O888BOB8OeB888B8B8B8B8B888B8B8B888B8B8B8B888B8B8B8B8B8B8B888B8B8B8B8B888B8O8B8B8;                                                                                                                    
                                                                                                                                                                                          c8B8B8BOe8B8g888BGB8e88888B8B8B88888B8B8B8B888B8B8B8B8B888B8B8B8B8B8B8B888B8B888B8B8B8B88BB8B8Br                                                                                                                  
                                                                                                                                                                                         C8B8B8GZB8B8e888B988B88888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8y                                                                                                                
                                                                                                                                                                                        b8B8B8ODB8B868B88BB8e8B8B8B8B8B8B888B8B8B8B8B888B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B888B8B8B888B8B88V                                                                                                              
                                                                                                                                                                                       c8B8BOOBB8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B888B8B888B8B8B8B888B8B8B8B8B8B8B888B8B888B8B8B8888B                                                                                                             
                                                                                                                                                                                      v8B8Bt08B8B8B8B8B898eeB8B8B8B8B8B88888B8B888B8B8B8B8B8B8B8B8B8B8B888B888B8B8B8B8B888B8B8B8B8B8B8B8B88888B8w                                                                                                           
                                                                                                                                                                                     :8B8BcC8B8B8BBB8B8B8BeB8B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B88888B888B8B8B8B888B8B888B8B8B888B8Bw                                                                                                           
                                                                                                                                                                                     8B8B2k8B8B88BB8B8B8BeB888B888B8B8B8B8B8B888B8B8B8B8B8B8B8B8B888B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B88888                                                                                                            
                                                                                                                                                                                    NB8BZN888B8Be8888B88BB88888888888B8B888B8B8B8B888B8B8B8B888B8B8B8B8B8B888B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B888v                                                                                                          
                                                                                                                                                                                    88B6tB88B8BeB8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8B888BB9O0#qD0Og8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8BX                                                                                                          
                                                                                                                                                                                   N88ZA888B8BgB8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8OGZp4wrLyrMhMwAhAwwUUct0ZOB8B8B8B8B8B8B8B8B8B8B888B8B8B8B888886                                                                                                          
                                                                                                                                                                                   8B8lDB8B8B8B888B8B888B8B8B8B8B8B8B8B8B8B8B8BGjHALvvvvvvvvvvvvvvvvvvvlvYVLyAyLyyvVvvYAmGB8B8B8B888B888B8B888B8B8g                                                                                                         
                                                                                                                                                                                  YB8UjB888BBe8B8B8B8B8B8B8B8B8B888B8B8B8B888DHThLyv;::,,.,:;:;;;;;;;;;;;;v;;;;::;vvhH2pbjbjZ88B8B8B8B8B8B8B8B8B888                                                                                                         
                                                                                                                                                                                  e8OwB8B8B8g8B8B8B8B8B8B8B888B8B8Bge88BqctcHq@ZCtC#C0tkAVvv;;;;;;;;;;;;;;;;;;;v;yHbOe88B8B8B988B8B8B8B8B8B8B8B888B8                                                                                                        
                                                                                                                                                                                 ;8BMg8B8B8e888B8B8B8B8B8B8B888B8B8twv::,,;;VwtC9GBB8B8B8D2Ny;;:;:::;:;;;;;;;;;vyUCCD#DOe6gegCmB888B8B8B8B8B8;eB8B8B;                                                                                                       
                                                                                                                                                                                 A8Cc8B8B8B888B8B88888B888B8B888B89y,.       .:;vlvrAwwcmkwMYv;;;;,;:;;;;;;;;;;;vvvv;v;v;;;vvVvwB8B8B8B8B8B8Bw   . v                                                                                                        
                                                                                                                                                                                 G9L8B88888B8B8B8B8B888B888B8B8B8Btv,..             ..,:;,;,:,:;;:;:;:;;;;;;;;;;;;:,,:;;v;v;lvv;Z88B8B8B8B8B8Bv                                                                                                             
                                                                                                                                                                                 ODg88B8B8B8B8B888B8B8B8B8B8B8B8BZl,           ..,.,,:,;;;;;;;,,,;:;:;;;;;;;;;;v;;:;;;;;;vv;;vY;Y8B8B8B8B8B8B8B;                                                                                                            
                                                                                                                                                                                 keB8B8B888B8B8B8B8B8B8B8B8B8B8BDv.           . ....,.:;;:;;;v;:;,,,::;:;;;;;vv;;;:,.;;;vvvvvvV;;98B8B888B888B88                                                                                                            
                                                                                                                                                                                 9B8B8B8B8B88888B8B888B888B8B8Bet,               ..vvv:;,..,:v;;:;,,,::;;;;;;v;v;:.,::;vVh4cUmv;;mB8B8B8B8B8B8B8B                                                                                                           
                                                                                                                                                                                 B8B8B8B888B8B8B8B88888B888B8BeDc.          . ..:;vywvYUptZU; .;v;;,,,,:;;;;;;v;;,  l088BBAHZkyvvv8B8B8B8B8B88888V                                                                                                          
                                                                                                                                                                                c8B888B8B8B8B8B8B8B8B8B8B8B8B8DOm.          .;:, ;kH ;88B8p#B8l.;v;:.,,;:;;;;;;v; ;GB8UvrBb b8m;;;#8B8B8B8B8B8B8B8,                    ..:,:,                                                                               
                                                                                                                                                                               v8B8B8B8B8B8B8B8B8B8B8B88888B88ZDU.             v8B   c8MBBMr8Bcv;;:.:,,,;;;;v;;;l,.B8OZB8B8 ,B9;:vUB8B8B8B8B8B8B8B8                 .,..,,::;:.                                                                             
                                                                                                                                                                               8B8e8B8B8B8B8B8B8B8B8B8B8B888B8#OX.         .   .l@#  8BCB8B8B8,O;,,..,.;:v;vvv;vvv GB8B888B68r.vvvvBB8B888B8B8B8B8BD             ....,:;:;;:.,:v,                                                                           
                                                                                                                                                                              cB8B8B8B8B888B8B88888B8B8B8B8B8BOOU.          .     ,rvmee9OBB8Crh..: ...,:;;;vvv;;;;;;;vvVvvv, ;;vvvC8B8B8B8B888B888BL          ., .;,.::;;:.:;;;A;                                                                          
                                                                                                                                                                             ,B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8@ZN,         ....,,.  .        .. .,   .  .,,,,;vv;;;;       :,:;v;v;U8888B888B8B888B88.       ..,,;;:,::;;:.;;vvvlU       ;v.                                                                
                                                                                                                                                                             #BB8G8B8B8B88888B8B8B8B888B88888B6CU,        . ....,,::;,::::,,,,,,;,  ,.   .,,:,,;;vv;;;;;;:;:;;;;;v;v8B8B8B888B88888B8B     ,:..,;, ;vv;:;;;;v;vYvMY   . ,vL;                                                                
                                                                                                                                                                            mBe8eBBeB8B8B888B8B8B888B8B888B8B8B#k;       .   ..,,,,;,;:;;;;;;;:;;;      .,,;;vY; vlvvv;;;;;;;;;v;vv;O8B8B8B8B888B88888B  ..,,;;;,:;;;;;;;vvvvVlYvmp  :.;v;                                                                  
                                                                                                                                                                           v8D8BDB888B88888B888B8B8B8B888B8B888Ztv. .         ..,,::::;:;;;;;;;;vA,  vO2;,vlVVmkYVvvvvv;;;;;;;v;;;v:rB8B8B8888888B8B8e8Bv ,,:,.,;;;;;;;;vlVlrYvyjt;  ,,,;,.vM                                                               
                                                                                                                                                                           gGGB89BB8B8B8B8B8B88eB8B8B88888B8B889pV...        ..,,::;:::;:;:;:;;;;YVllv,. .,;:.;yyYvvvv;;;;;;;;;v;;;;;8B8B8B8B8B8B8B8B8B8B; ::;;v;;;;;vvvvvlrvVhk;    ;,.:YYvlv                                                              
                                                                                                                                                                          6bp68e8e8B8B8B8B8B8B8B8B8B8B8B8B8B8B860A; . .         .,:;;:;:;:;:;;;;;,.,,      ..    .::;:;:;:;:;;;;vvvv;c8B8B8B8B898B888B8B8G  :vv;;;;;vvvvvvVVwL;      .; ,;;vvv                                                              
                                                                                                                                                                         @9mp888e8B8B8B888B8B8B8B88888B8B8B888B8@X;...           ,,;:;;;:;,;:;;;;;,.    ;Yv:.VHL. ,;:;;;:;:;;;;vvv;l;yB88888B8B8B8B8B888B8X .;,:;;v;;vvlVMMy.         ;,.,;vvvv                                                             
                                                                                                                                                                        VBHb8BeeZB8B8B8B8e8B888B8B8B8B8B8B888B8894; ..          ..,;;:;;;:;:;:;;v;. .Vg88B8B9D9B8O;:v;;:;:;;v;vvvvvv;v8B8B8B8B8B8B8B8B8BB88w ,;vvvYvvvyrUv            ,:,.,;vvl;                                                            
                                                                                                                                                                        82m9B8BeO8B8B8B8e8B888B8B8B8B8B8B888B8B8etv,....        ..,;;;;;:;;;;;;;;rq8BZl.   ..   ,tBeTv;;;;;v;vvvvlvl;vB8B8B8B8B8B8B8B8B8B8B8v:vvvv;lywr;               v..,;;v;v                                                            
                                                                                                                                                                       9em0Z8G8@GB8B8B8BeB8B8B8B8B8B8B8B8B888B8B8#y.,,,.. ... ..,,::::;:;;;;;;v;vr8O,  .;;vvYvVVv:Lwrvv:;;;;vvvvlYvvvl8B888B8B8B8B888B888BB8G;,vvvYMM;                 ;; ,,;;;;;                                                           
                                                                                                                                                                      h8A09B6B8Oe8BBB8B8B8B8B8BB88B8B8B888B8B8B8Bgh;.,.,.......,,:,;,;:;:;;;vv;v;,.  ,vvvVlYvvvvvv::;;;;:;;vvvvvvVvv;wB8B8B8B8B888B8B8B8B8B8Bj.:VTr;                    ;..::;;;v;     ,,:,;;;;                                             
                                                                                                                                                                     ,8#w#99OBBO8B8g8B888B8e8B8B8B8B8B8B8B8B888B8Bbl:.,,,,,,,,,,;,;,::;;;;;;v;v;:;..;:;;v;;;;:;;;:;;:,::;;;;vvvvVlLvvO8B8B8B8B888B8B8B8B8B8B8Bblr,              ..,;v;  Tv,;;;vvvv, ..,..,:,;;w                                             
                                                                                                                                                                     88mk@b8q8g6B8BBB8B8B8B888G888B8B8B8B8B8B8B88894;,:;,:,:::,:,:,:,;;;;;;;;v;;,::;;;::,,,,,;::,::;:;;;;;;vvlvYYVVvA8B8B8B8B8B888B88888B888B88.               .   ,:;vy4Mvv;;:v;;vv ..:,;:;vv                                              
                                                                                                                                                                    tBDAZe9BOBZO8B6e8B8B8B8B8BeB8B8B8B8B888B8B8B8B8by,:;;::,:,,,:,:,::;:;;v;v;;;;:;:;::,,,,.,,;,::;:;;v;v;vvlvVvyVlYgB8B8B88888B8B8B8B8B8B888B8v             :l;    ...,:;llv;;;;;;;;:;:;:;Yl                                               
                                                                                                                                                                   yB8kU9eB8B8C888G8B888B8B8B8e8B8B8B888B8B8B8B8B8B9my::;;::,:,:,:,;:;;;;v;v;;;;:::;,:,:,:,:,;:;;;;;;vvvvvvYvyyLyyLeB8B8B8B8B8B8B8B8B8B8B8B8B8B8.            ;v;vy;    ,...;:;;;::,::;;v;;Vv                                                
                                                                                                                                                                   B89TjGD89Be688BeB8B8B88888BB88B8B8B888B8B8B888B89Dty:;;;;;;;:;;;;;;v;v;;;v;;;;:;::,:,;,;;;;v;vvvvvvYlylylAyyyVA8B8B8B8B8B8B8B8B888B8B8B8B8B8B8           ;   :;VAV.  .,:.,,..,,::::,:;lr                                                 
                                                                                                                                                                  B8Bjh##DeZBeG8B888B8B8B8B8B8B8B8B8B88888888888B8B8GDmy;;;v;;;;;v;;;vvvvv;;;;;;:;::::,;:;;vvvvYlYYylVVyrrrryTyLc8B8B8B8B8B8B8B8B8B8Bg88B888B8B8Be         ;   ....;vhr,  ,:.:,,,:,:,;:,,l                                                  
                                                                                                                                                                 X8BGtc#OB96Dq8B8Z888B8Bge888B888B8B888B888B8B8B8B8B8Z@tNvv;;;vvvvvvvvvvvvvvvvv;;;;;;;;;vvvvyVyVryrLyVyVAyAyrTtg8B8B8B8B8B8B88888B8BB98B8B8B88888Bj       ;     ... ..vM;  .:,:,::::;:;:;                                                   
                                                                                                                                                                v8BecwtO6BO80OB8B6B88BB86BB8BeB88888B8B8B8B8B888B8888BODpmAVvlvvvlvlvvvvvvvvvvvvvvvvvvvyVrATMwAwrrrALryryLYLk8B888B8B888B8B8B8B8B8B8BeB8B8B8B8B888By      ywv;     ,,..,:;,,.,,,,::;;:;;;.                                                  
                                                                                                                                                                888Bj4GpBGeBC68BGB88BB888B888B8B8B88888B8B8B8B8B8B8B8B8O@pc4AyyvlvYvvvlvvvvvlvvvVYyVLyAAMrMAAATrrVrVyyLyrMcB8B8B8B8B8B8B8B8B8B8B8B888eBB888B8B8B8B8B:     :v;yl.   .,,,,.:;.,,,,:,::;:;;v;                                                  
                                                                                                                                                               8B8Bg42DbeeG8t8B8q8B8G8BBB8B8B888B8B8B8B8B8B8B8B8B8B8B8B89ZbjHkTrlYlvvvvvvvvvvYvYYyVyyyVVVyYVlyVVYVYVlyyce888B8B8B8B8B8B8B8B8B8B888B8B8O8B8B8B8B8B8B8B,   :   :;wv.   ,.:,:,,.,,:,:,;:;:;;vv                                                 
                                                                                                                                                              0B8Bebk2#C896BCB8e#B8B6B8BgB888e8B888B8B8888888B8B8B8B8B8B8BObjmHXAArlylYYlvlllvYYVlylYYyvllylYllvVlyrCG888B8B8B888B888B8B888B8B8B8B888BBB8B8B8B888B8B8B   ;    .;YY;  .:.,.,,:,,,::;;;;;;;;v.                                                
                                                                                                                                                             jB8BBZbUZD6B0gg28Be#8B8G8B8O8B8BBB86BB888B8B8B8B8B8B8B8B8B888B8eO@tmXrylylvvvvVlvvVlVllvVllvvvYlYrUceB8B8B888B8B8B8B8B888B8B88888B8B8B8B8B8B8B8B8B8B8B8B8B         .;yV   ,.,,:,:,::;;;;;;;;;v;                                                
                                                                                                                                                            kB8B88jHkCZ88C8ZOe89G88GBBB6OB8B8B8B6e8B88888B888B8B8B888B8B8B8B8B8B888B888Z#Ct4XUUrMrryArwUcjq#9B888B8B8B8B888B8B8B8B8B8B8B8B8B8B8B888B8B888B8B8B8B8B8B888BZ      .,,:vv  .,.::::;:;;;;;;;;;;v:                                                
                                                                                                                                                           M88B8G9kjcOZ8C#9#B8e9B8BeB888O8BeB8B898B8B888B88888B8B888B8B8B8B8BOj88GG8B8e8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B88888B8B8B8B8B8B8B8B8B8B8B8B88888B8B8B888B8B8B8B888.    ..,,,,;.,,,,:,;:;;;;;;;;;;;v;                                                
                                                                                                                                                          UB8BBB8DUkGOO82eeO8B8GB8Be868B8B8B8B8B8B8B888B8B8B88888B8B8B8B8g8BqZ8BcC8Bby4cHUjO2ycCpew;p98B8e6G8B888B8B8B8B8B8B8B888B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B888B8v  . ..,,:.:.,,;:;:;;;;;;;;;;;;v.                                                
                                                                                                                                                         mB88BOBB2U29D98@g6GB8eeB8BOG8B8B8e8B888B8B888B8B8B8B8B8B8B8B8B8B89D88BDc8BpN8#jtb#wT0UmMMA vv46qGv2BBB888B8B8B8B8B8BBGGDZDO88B8B8B888BBB8B8B888B8B8B8B8B888B8B8B8B8:   .,,,,:,,,::;:;;;;;;;;v;v;v;                                                 
                                                                                                                                                        ZB8BGB8B9CtpOOeeZGZ98BBe8B8B8B8e8BBB8B8B8B8B8B8B8B8B8B888B8B8B8B888B8BeN8B8bg9@tj0gBe9@mNmOXN;AOjvUG888B8B8B8B88eDCHHXNMTrLr8B8B8B8B8B8g8B8B8B8B8B8B8B8B888BBB8B8B8B: ..,.,,:,,,::;:;;;;v;v;v;v;;;v,                                                
                                                                                                                                                      L8BeBB68BeZbm#qe8OZG@G68egB8B888B8B8e8B888B8B8B8B8B8B8B88888B8B88888B888k8B8BCA6Dp#B2vvkO20CrHtMmmmAUGB8GGOOO6O9CcMrVvvlvYlVvyB8B888B8B8B888B8B8B8B8B888B8B888Be#Oc2B8  .  ..,,::::;:;;;;v;vvv;vvv;;;vv;                                              
                                                                                                                                                    ;B8DeBBe8BBeOc#O0ZBZBGDOBBBO8B8B8B888BBB888B8B8B8B8B8B8B8B8B8B8B8B8#8B#H88HB8BBB9G9OV;C,  ;Zjj2CjHmU@yye89ZDD0Z#jUAvvvvvvvvvvvvY888B8B8B8B8B888B8B88888B8B8B88888B8e9ch.  ..,.,.,,,,::;;;;v;vvvvvvv;;;;;;;v                                             
                                                                                                                                                   eB8e6B8O8B8G9tqBOZ8BOOZq9e8e888BBBe888888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8888Oy8B8BHceCjNtL .ympNbmh44hHwYc8eOpC@ZjUllvvvvvvvvvvvvvvG888B8B888B8B888B8B8B8B8B8B8B8B88888B8#  . ,.,.,,:,;:;;;;v;v;v;vvv;v;;;:::;;,                                           
                                                                                                                                                 O8B8Z#BBB8B8BO9@t6CCe82ODG988B6eB8B8B8B8B8B888BeB8B8B8B8B8B8B8B8B8B8B888BeB8Blp8BjcgjbUqBODq#bAcrrVvhO#bUC9DtjjcMl;;;vvvvvvvvvvvv;:@B8B8B888B8B8B888B8B888B8B8B8B888B8B8BG .,,,,.,,::;:;:;;v;v;vvv;vvv;;;;:;:;;;;                                          
                                                                                                                                                B8OB6bG8B888BG@8Ct600eBZO969B8BGe8BBB8B8B8B8B8B8e888B8B888B8B888B8B8B8B8B8B888BO88B8e8BBB888BeeGGeeO#6ZGeBB8jCkrv;:;vvvv;vvv;vvvvv;;c8B888B8B8B8B8B888B888B888B8B8B8B8BBB8Bl .:;,,,:,::;;;;vvv;vvvvv;;;;;;;;,;;,,;;:                                        
                                                                                                                                              AB8#88@q6#8B8Beq9Gcq8Z688CB99e8B8G888BBB8B8e8B8B8BeB8B888B8B88888B8B8B8B8B868eeZ89eB8eD9B9ZZq2qD@jj0@C#pbjpttcj@cryvv;vvv;;;v;v;v;;;v:mB8B8B8B8B88888B8B8B8B88888B89#t9pH#bUDH.,v,,,::;::;;;;;vvvvv;;;;:;:;:;;;;;:;,:;;                                       
                                                                                                                                             UB898eZ9eG8BeB8GDBZcDDj98B#9eB8BO8e88B8B8B8BBB8B8B8B888B8B8B8B8B8B8B8B8B8B8eB8B9B6Bee6B9eZ2@OC2CZ2jj#2pb2mk2Zbpmt0bUhrv;v;;;;;v;v;v;;;vUZt8B8B8B8B8B8B8B8B8B8B8B888BecVwbyvrl.:;v:::;;;:;;;;vvlvv;;,,,::;:;;;:;;;:;;;:,:;.                                     
                                                                                                                                            cBeGBGGDO98B8B8B9O8bpOOZ8BGZD68B8G8O8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B888Bg6gB9Bg6eeZb0pD@0cjp#2tj0pCptb0bCccmHHpHAyvvv;;;;;;;v;v;:vArY@8B888B8B8B8B8B8B8B888B8eD0DhvLr;v;;,;;;;v;;;;;vvvvvv;,..,,::;:;:;:;;;;;;;;;:;:;;                                    
                                                                                                                                            LMt8B8@eO8BeB8BBZee@2O#8B8gGqeB8B88DBeB8888eB8B888B8B888B8B8B8B888B8B8B88888BeBe9ee866O6O@CZ#0bqcttc#tjjH0C2jqjkj0ccjCmAvvv;;;;;;;;v;;;;v;;yMtbeB8B8B8B8B8B8B8B8B8B0cNrUvvv;;:;;::;;;vvvvv;v;:.. ..,,:,;:;:;:;;;;;;;;;:;;;:;;                                   
                                                                                                                                            vq8889G98Bee8B8B9BemCG0G8B8BOB8B8B868B8B8B88888B8B8B8B8B8B8B8B8B888B8B8B8B8BBe898GBZGZOCOZqCqqZbq2ccjcbCpc0pkt#ttjqjpp##mYvvv;;;;;;;v;vvv;;;;;;;VyXqC4c##Oe2Oq9BOG8Zj4y;v;;:;:;:;;;;,.,.,.. . ..,,:,:,::;:;;;;;;;;;;;;;:;;;,;v                                  
                                                                                                                                           ..VNgB8B888B888BegBbbe9O8e8BeG8B886e8B8B9B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8Be8geB98gBG999OO@#0qCCCbjZpbHtcDqHttmttttCmmtjcbDG4vvvv;;;;;;v;vvv;v;;;v;v;Vrv;v;::;.:;;;;;vvvv;;;:;:;;;;;;;...........,,,,:,:,:;;:;;;;;;;;;;;;;;;:;:;;                                 
                                                                                                                                          ,     U#eB8B8B8B8Be6Cq8cGBeB8GBBBB8B8B8BeB8B8B8B8BgB8B8B888B8B8B8B8B888B8BB98g8GBB9eGGGOq@#bCpCtjtppj2pkptcb#HmtbtjtcmCmkckp9Hlvv;v;;;v;vvlvvvvvvvvvvMA,  .  ,..,.:.....,,::;:;:;:;;;;,.,.,.,.,.,.,.,,:,::;:;;;;;;;;;;;;;;;:::;::;                                
                                                                                                                                         ...        v#B8B8BBB8jZebO8e8B888G8B8B8B8B8B8B888B8B8B8B888B8B8B8B8B88888B8eG8Be9B6O9O6ZZq0Z#ttC2j2ctUcUbOtm2HmcCjmcptjpCtjUjjDprv;vvvvvvvYyLvv;vvvvYycv    . .., ,.....,,:,::;:;:;;;;v,,,,.,.,.,.,.,,,,:,;:;;;;;;;;;;;;;;;;;:;:::;;                               
                                                                                                                                        . . ..  .     ,28B8Be#CBeHOO9B8B8B8B8B8B888B8B8B8B88888B888B8B8B8B8B8B8B8B8BBBBeG9BgB9Z##t0p0CCctjp2jc2ttcct2tUmbjbkt2pjbHCjjpmbq#tVvvv;vvYlLLrVv;vvYvhH,         . ......,,:,:,;;;:;;v;;,,.:...,,,.,,:,::;:;;;;;;;;;;;;;;;;;;;;;;;:;.                              
                                                                                                                                        ......,. ,.      HB8B886#296B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8BeB86e6GOe9B6GDZZDpCq02@tpkmcCqqbjjjUjHbccpmHbmjtcHctcc9mb90Uy;vvvvvYAA4yvvvvyXL.          . ......,,,,::;:;:;;;;;,,:,.,,,,,,,,:,::;;;:;;;;;;;;;;;;;;;;;;;;;;;                              
                                                                                                                                       ,  ..,.,;, ,.:.      yp6B8B8B888B8B88888B8B8B8B8B8B8B8B8B8B8B8B8B8B888B898GeGeOBOGeOO9qZ2DGO@#ZO0@Cbm2cmctttpCcjttj2Ujpccm0cmDND#jCCCGpy;vvvvyVAAwrVvvAUv,.,       .   . ....,.,,:,::;;;;;:,,:,,,,,:,,,:,:,::;;;;;;;;;;;;;;;;;;;;;;;:;;:                             
                                                                                                                                      ..  .  ..;v: .:,.,.       rBB8O;w0M6BBOZD8B8BO688888B8B8B8B8B8B8B8B8B8B896BeGG99GeOeOe9BB8B9G8O@bZO8BGeOc0mcptjcjpUqCHct2HmZmCpjttbHtCjCOtvvvllyyAAUMVlMAv,:,. .           . ,.,.,,::::;;;;;,:::,:,:,:,:,:,;:;;;;;;;;;;;;;;;;;;;;;;;;;:;;                             
                                                                                                                                      .     . ..;v; .:;...,..            :;;:.,,::;.;;vrkUwH8eBBBe8B888B9eg6e6OeeDGD968BeB8BeCV..,.       .;wpgeBGtkjcpUCbkc0hbjtbcjk4UpZ2pkO2#BOAvvVVyyrAhAArAv;;:,,...           ...,.,,::;:;;;;;,:;:,,,:,:,:,:,;:;;;;;;;;;;v;;;;;;;;;;;;;;;;,                            
                                                                                                                                     .         .,;vv. ,;,........        .,,...,...,,;;Vv;vgBZC9ZGOOeOZOOgO9OgZ#ZqCeB8B8#L:.                   ,rbB8DpkjcUkpmCOUcH4C#bmt2ck9GmUjpBqV;lYyyrAXTTrAlv;;,,.,.... . . . ......,,::;:;;;;::;::,:,:,:,::;:;:;:;;;;;;v;v;;;v;v;;;;;;;;;;                            
                                                                                                                                                .:vyY:..;,,.,.,.;...,...,...,..,..,,,,;:.vb9DbZCp2CbOt0#@bCqq@q2#B8Bm;                            .UGBeZttcbmjCHt@2NCb2HDjc2ZcqC9@99NvvlyL4UUMw4Ulv;;,:,,.................,,::;:;;;;;:;,::::::::;:;:;;;;;;;;;;v;;;v;v;;;;;;;;;;;                            
                                                                                                                                    .          ..::vVV;;:;;:,:,,,,.:.,,,,:,,,:,:.,...:  vD@c0qmpqq0D2tZ#tCj@c068B0;                .wM;,             vj8GCm#mjbcUOjLCqcjj#2c#ZC0ZcbGOCvvVAMXXXMkHrlvv;,:,,.,.,.,...........,,::::;;v:::;:;:;:;:;;;;;;;;;;;;;;v;v;v;v;v;v;;;;;;,;                            
                                                                                                                                              ..,,;;vvrYv;:::,::::;::,::;:;:;:;;;,,.,, .eeC0pmCtcHpCtcpmOHc#6BeN          XB8A      rZ8B8B60cv         .jBZ2Zt0mtbtjZc#CcOct6CqOC26#OeGUYywTHk4NjUhyYv;:;:;,,.,.,.........,,,,:,;:;;;;::;;;:;;;:;;;;;;;;v;vvv;vvvvv;v;v;;;;;;;;:                            
                                                                                                                                               .,::;;vlrryvv;;:;:;;;;;;;;;;;;;;;;;,..  ZGZmj0CCCHtc2cCbC2jB8C:        .l88BZ,          ;p8eOgBGN      .  ;B9qpmmHBcH@m#bC9t0D2COZq#e##6emrYMXcHUc2mkAYvv;;:;,:,,.,,,.,...,,,,,,,,::;;;:;;;;;;;;;;;;;;;;v;v;vvvvvvv;v;v;;;;;;;;;;                            
                                                                                                                                              ..,,:;;;vvr4HwMVv:,:;;;;v;v;v;;;;;v;v.  9BjpjmjjcmptjcptjcCBB;        q68BB.               .CqcH2OeOw        O8#Zt@bcCpCCj@cZ#Zj@#9Z2Z#ZGOeqUyXmmXmjCmkAyvv;;;;:;,:,:.,,,.,.,.,.,.,.:;;;;;;;;;;;;;;;;;;;;;vvvvvvvvvvv;v;v;;;;;;;v:                            
                                                                                                                                             ..,,::;;vvllMkcjpcHVv:::;;vvv;v;v;vvvv  kgccmj0Tkc0pjtcjcm#8H       vZ89gO;                   #OmkmtZ98t       082CjUZOqZcb2D@ZO0#9DZ#OGZO9OB9twUHcHbbtHHwLvv;;;;;;:;,,.:,,,:,,,,.,,,,:;;:;;;;;;;;;;;;;;v;vvvvvvvvvvvvvvv;v;;;;;;;                             
                                                                                                                                              .,::;;vvvvVywUtjZ98Bebhvv;;;v;yyVvvvr lCjmtHcjCmcmkc2pbmGB       vBBGb8t       .HC;;8DV       mBDwjccHeel      H80jj#jZjGZbZ@ZZqOZDZO#9DOOOOBBZktjjC@bjttUAvvv;;;;;;;::,:,:,,,:,,,;,:,:,:;;;;;;;;;;;v;vvvvvvvvvvvvvvvvv;v;v;;;v;,                             
                                                                                                                                             ..,:;;;;vvYVAMUkjpZ9BB8B8BejjhmHtC0UyUAjpmcq2HjCjj0tmcCreB.      c8CmpBV       beG6B898B6r      0BkUtcUkbBA      qg#O2ebC9@qp9@0GO999O9D9OGODOg8eHb2q@9O@tjkMvvvv;;;;;;:;::,,,:,:,:,:,:,:,,;v;;;;;;;;;;vvvvvvvvvvvvvvvvvv;v;v;:;v                              
                                                                                                                                            ..,,;;;;vvYVrA4Ucc2COOG68B888B8B    VBZjCUtmU2mkpjmmCttm9B       BGkkH8: .    rB8eeBBOgO  cB      2etNtHkHOBY      9GbpZCCHbZjqDGCO0ZO9D9OGO9G968Bepq@Ge8gGODpXylvv;vvv;;;;;;:;:;:;:;:::;::.:;vvv;v;v;v;vvvvvvvvVvlllvvvvvvvv;;;v                               
                                                                                                                                             ..:;;;vvVYLrhNmmtjqDOO6e888B88m    AebUjmptccUc4tp@kHjCB       eDctc8V      qBOjm;        ;v      9qcmtjkk#B      l8Z0kDZj0bOtDZZ#9DOO9O9O9O6ee9BBeO968B8B88BDCMLvlvYvvvv;v;;;;;;;;:;:;;;:,,;;v;vvv;vvvvvvvvlvYlYlYvlvvvvvv;v;v                                
                                                                                                                                             .,,;;vvvlyyMMUkttCqZZ9gBe8B8B8: ;t88ekwNjc4CjkUcmjmkU2BX     .BZmtbOj                             ;8jkmtpkX9O      p8qjZq6jOD###9DO9#O9G99O99eGee6B8B8B888B8B8B8qUVVvvvvvvvv;v;;;;;;:;:;:;:;:;;vvvvvvvvvvvvVlYlVlylYlYvvvvvvvv                                 
                                                                                                                                             .,:;vvvvVVLAh4mmpbZ#69eB8B8BeYlZ8B8B8krhH4TUHmNXmUcjtq8      Z8kccZe      ,                   .    N8cHmCUDX8c      8O#jZbj9OCDZO#OOOO9OGOGGe6e6eG8B8B888B8B8B8B88GmMVlvlvlvvvv;v;v;;;;;;;;;;;;;vvvvvvvvlvYlYlyVyVylVvlvv;v;Y                                  
                                                                                                      .;v;vvY                               .,:;;;vvYVyrhXmmjp#qGD888B8O;  ,vv.  COrkhkH4XmXkckkcjey     v8HjXc8v      B. ;      hppON;  Y       H8NHMt0Uj8      N8@26Ct9#OZOZOZO9GOGOGO9Oeegg88888B8B8B8B8B8B8B8BGUAvvvVlVvvvvvvvv;vvv;;;;;;vvvlvYlVlVYVVyyryryrVVvvvvv;                                   
                                                                                                       :vv,,XH                             ..::;;;vllrAhXHmjpC#O9888Bj,.. ;G88B8BtlTwTkjMtcmHkUUwOB      g@HmkB0      X8,   h,       4B988b       L8qmpUjpZc  .  ;8ZH#cDZ@0OD#ZOZ99OO99gGeGgeee8B8B8B8B888B8B8B8B8B88B#HryvVvvvlvYvvvvvvvvvvvvvvvVVyVyVyVyVyyryryrylvv;                                     
                                                                                                      .,v;,vrX                    .         .,;;;;vlyrw4Hc2cCZZ68B8U. ;.,B886e8B888whcmUtkUcUwmmH0G     teyttOG       BZqHcD8A       y8O9@8y       yeBjX#m@8      geObCOj@DDO#9O9OOO999O8GBGGe8B8B8B888B8B8B8B8B8B888B8B8B@k4yYvvvvvvvvvlvlvvvv;vlyyLVrVyYyyMyyAyYV;,  t                                    
                                                                                                     ,..:YvYVy                              .;;;;vvyLwNHH20Z#BB8By  ,lV jg yA@bCB8B8Hwhm4MT4UtNhmer     eqH2OD       2B0###6By      08bl  L0 ;      ,ZGZHc98      q8b2jG2Z0C#9DOZ9DGOe9GeOeBBB88B8B8B8B8@9B8BCOG689888B888B8B8BepcMyvylvvYYLVylLvvvVLyrrYrYrlyyvv.  ;;  j                                   
                                                                                                         vAVTY                    .         ,,;;vvlVrTkHjb#e8Bg;  ,;Vv ., rY;keB8B@OgyUtUkNrkwwkUBA     BpO8N        e89GeBBZ;      .      :  ,       UB8#CB      OBjCtqjpZD9OOZGOZOO969996BBe8B8B8B6B9cZB8B8B8GOZeB888B8B8B8B8B8B8B8GZHhVlvvvv;vvVyTrhvvvvllvvv,:qB8GCT9B                                  
                                                                                                         ,;:;y                     .      ..,;;vvVlrAUHp08B8j   :;;Avlv   , Z8B8gmHjDeCAhjUcUNmjX9.    vB8@                                    .        Vpe8      c8CCc@bGZO#DDGOG96OGG6B8gBg888B8jvvvk888B8B8CZBCB888B8G0O888B8B8B8B888B8B8B8Bg9ZcHXw;kc2r .  MV8B8B8BcmBw                                 
                                                                                                   .     .v: v                    . .   ..,,;;vYhXkkctqB8Bk   :;;;VLy       eB8BUUCb0ZGZpXrhHmUMCe,     v          v              ;          y  :  .      .;  .   bBbtZq99ZOGDGGOOGZOgGOgeee8B8B8Gvypq8B8B8B8B8Be08B8BG6@68B8B8B8B8B8B#p98888B888B8jqC8B8B8N6pAy9B88888BHc8X                                
                                                                                                  .:;      .:v                    .. . . ,,;;;vAj#bZ@BB8;  .:vvvvYymC;   vrvC8BCkCCCbbtZOtykHXhmNB,   .           ,0                .,., y; v8k  .     ,          C8pC@j09ZO9DOGO9ZDOB99OeB8B86cNv@8g8B8B8B8B8B888B8B8m2B8BeH8688Oe8B8cyhwrbtLp8B8B8jOG8B8B8BU4t#tO8B8B8G9B8                                
                                                                                                ,,..,;;     ;v                    . . ..,,;:;;;vwcOB8Bv  ,;:;yrvVAA4kTv:mchU8BAA2bbpCCcm8BTVmhmt46;    .          ch                888A ;DO8Zel                  BOpDGZOOOOGZGDOOOOG9GO8Be0jmHtbO8B8e8B8B8e6B8B8B888B8B8B8BbB8B6B8B8B8BCqkrAweB8B8BkBtt8BAc6UOq;yCtm88B8jk                                 
                                                                                             .   ..  ..,,;,::r                    ..   ,;;::,,.;;vH80, ,;vvyYYvyXkHcMc@.vt;:;GTMC@Cbp@tcBBrHAhktUO:    ;6        ;BeM    ;wqBX     .BmA      BOe           v      e@b6#OOGD9ZOGOOGqOOOB8Otkcjq#O98B8BBB8OCXrNeB8B8B8B888B8B8jhG8k#B8B8BB#qUcyv#8B8B8gGBmOck8;vy#L   vtD8B;                                  
                                                                                              ..    . ....;,;H                     .  .:;;;,..:,   yChTrlvLlvvAht4pgZH8r,,;vc8Brtp2CC2pm8BUkcXATk9O     8v       XBtwB8j8BZC8M        ;      .69@v        #T     r89OODOOOODOZ9#OOGZGBejmcCZeA AUTkhUkHck;Me8B8B888Bj#2cUb99888B8jTC2B9TpBCvv tB8B88jBXyByOM .  ;;.  ,GBq                                   
                                                                                                       . ...:H                     . .,;;:.: ::     2DHtXmmXVLhM4pB8B9;cv O#pBBNvXX2bbtcB8MNmtUXMcg;    g8      eZ9:  .He9hO8,      G8Bv      ;B28r    Y m8      pBOO9OOOOOOZOGG#ODee0cpc29B,   vH4cUbjNvqOOB8B8Bp,A@hyZ88U4chyVk6B8ZvT488B8k2B8B8mMk8ByGV6V :;: ;0;, Ab                                    
                                                                                                          . ,N                       ;;;:;;. ..    AODbkbtmtCjp@BBBX8B H,:Ockh8Bvyhkc2bcBBUNAmqwyr6r    ;B8UhNc9O4Ov     L8A       v8Bm        kqmBBv:ZB8B,     w9eZ9OODODOOOq69DOBg0HcCZeO   . YUqOkrUZ66#e8ObmYvTLkB8v2cG8888B8OUr88w;8B8B8B8B88Hv8bTe8w8 ;.ym vgyr;.                                     
                                                                                                             v                       :;VZ8BGA,    A2Ze0tkccCbq#BB8  e8vvlrcbbUv8qvAhX@bmZ8TUmcHckrbD     ABO@#bcc28;        lv     ,8H        .qOpHeO90HB,     ,B6ZDO9OOZODOZD9O0B9jkjOgBV  ;..v,UG96OGe98B8#ZCZeOAZB88UjkB8B888B8B8ygBDZ8B8B888B8BjvLB8BrD  ;tG,vm,;4                                      
                                                                                      :;yXc0BB8B8B888B8B8B@Amq                      :;keZvvc8g@tqe8B8ZbmUbpkGe8B82  j88;;rAHt  C8rrymq#mpB9y4AccmmUeL     DZtAjHkce,        8v .  .         L98B8jpCmgcBk      q8bOO9GOqO9OCZD#C8gcU2OGX.  :v;..,   vB888BBD8B888B8B884U89jB888B8B8B8O8B8B888B888B8B8B8B8BDr .Nb :8X:;                                      
                                                                              .VUCB8B888B8B8B8B8B8B888B8B888B8                      ,vv;.;v;lkb#0O69jCbCtjCOB8Blr2 .hZ8G vyrw vvyOyVTcCCm6BHVTmkkcck9;    v8qmtmkD60        G       gD     UB8Ov;tBcm6Bk      y8#O96DOZOGOpOOCO86tXZO@:   r; :XOB89B88GCcDZZZ6CM08B8B8B8B8B8B8B8B888B8B8B888B8B8B8B8B8B8B8Dyv,;8.p9O;,                                      
                                                                          v08B8B8B8B8B888B8B8B8B888B8B8B888B8B                      v;;vv;;vlvVrrVAyLHjtbD8B8BM M@  rB8Bw yy.;;:;w6yNXcbpjBZrAUXjXtT9b     lOttHerqBO     VB8       YBe:  ltr;    lGeB.      l8g0ZG6OOOCZ#O9DO8Zj0Ok   .;  ,XvvvAmeBqXwXmbcm8B9e8B8B8B8B8B8B8tpc8B8BeB8B8B8B9@899B8BBvye8BC,;v;m8p;vh                                       
                                                                       AB8B8B8BBBBe8B8B8B8B8B8B8B888B8B8B8B8B8                     vvvvv;vvYvYywkkw4tcUc08BX vheB8; :eB8Bv  c88A .B9rhkpptZ8NytjhUcjM8U     ;Bp92  BB8BAk8BO,                       U       m8GOqODD#9DqqqbODe2CZU   VBh , ;mgmYm8Ztc6B8B888B8B8B888B8B84yO8t; ;B8COBDCeB8B8B9e8B8BBG8qpBBj:vUCYXky9p                                       
                                                                    HB888Be6BeBe8eBe8B8B8B888B8B8B8B888B8B8B8B                  .vMvv;v;vYTVyrryXTMkmm0G8B, ;B9ZGB8h r888By .m8. rk.tNANqjp8elXHcHmmmXDV     ,BB      ;;                  .       ;        v86@9#ZGZ#DG@O@ZB89c.   vO89  ,rvv;8BUY8B8B8B8BB88wY68B8B888cU;,2m::v80OB#;vVt  y8B8B8B88Zq8B896C8BBe8B8B;                                       
                                                                 ;88B8eeG6ee6egBBBB8B8B8B8B8B8B8B8B8B8B8B8B8B8                .yrv;vvvvYvlMMylyTAUcjUD8D;vceBevHOqB8H4B8B8B; .Lv,.v yeVAmCHOBcykUHmjmXc9y     vp                                          p8gOOO#9O9@OZqZOe82mkvwCcCZm   mB.v8B6G8B8Bee8j :88vZ@h,,BBDUpZCb80v.HBeOCvVj;       XB8B.                                                        
                                                               T8B8ge6gGeeBeBe8e8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B              vLrvvvvllvlVrrMlVAU4tC@0O8h .D8B8hkXAtDB8C82:;8t :vr8er, b8VAHjUeB4YUHjkUHhHBm                                    .         ;BBO9OOC9Dq0ZZODO88VvwrMcCqU,  YlrCe8brv88Btpt82;;eBB2X   t8tmCq#88V.:.A8GpVlL9;                                                                    
                                                             0B8BG6e6e9eB8eBe8BBB8B8B8B8B8B8B8B8B8B888B8B8B888            vTVlvvvyvvvyyryLVrhkc2C0t#B8;.jeB6 .OejcjGqCt9Bq68v :jBhwU  v8cVm2m8DHkrhmktHUweD;                ,:rjUhX#CCmq#HMv;:         HBBDG9GOOOO0ZC99Ze8mvmjpty;   ;B8OGB8m  UB8O0H6B4XClt#;;, hB2mZ0CB8v ..vw..B88C;                                                                     
                                                           H8B8eBgeGeeBGee8e8e8B8B8B8B8B8B8B8B8B8B88888B8B8B88          AwyvvvlvllVYVvLMMLTTHHjpmHBBGVYDBw4v  :vNtZC9bMtDB8B8 vr ,Cr;  v#UrjCbBHYmbrHkMb#XZBC.           ;qB8egeGO6bO9BB8B8Y         v8BeOO#OZOOD0GZZCjG8c  .      AeB8w,B8Av  H8Oqmk68B6  B  y  68kqCZeO; .vvV   lBU                                                                       
                                                         :B8BBeBeBgeeeeBeBBBB8B8B8B888B8B8B8B8B8B8B8B8B8B8B8B8       vwXrvvlvvVwrMTAvVAwrXkmkmcpmeB;  mB889q     ,  leBt#8B8B8;,;,. vHVvj8ZUcjG8UUhUtHHHXmHcq6mv         ycO66geB8B8ejX:          .AB8O9#9ZOZZZ9@ZODGD#8t      :C8B8B6  A8A,  yG0ZCbe8BVv v8     8Obq9OV  ,;vL;,:vq8B8B,                                                                    
                                                        eB8eBe8eg6BeBe8B8B888B8B8B8B8B8B8B888B8B8B8B8B888B8B8B     vHMrLvlylVyAryvvlrATwUkHUttjCqBB ;jDq6B888vA..    XBeqO8B8B8XUYYvLOr;,vkbHkj#8mTkkHUwtUhcmp9Bb,                              yc8BebgODCODDZOZDZOZOG8@  t88e8B8B8w;. v8b v ,eBe66B8B8 l.:B  Y,cB#6GCr vX4kwlyk8B8B8B8B;                                                                   
                                                      T8B8BeeBBB6BBBeBB8B888888888B8B888B888B8B888B8B8B8B8B8B8,  UpcTyVVlLyryyvvvlYrr4hkHmktqO0eB8O kGhXO888Y HU2y    ;peBB8B888e88CBZvw;;yvmbUtOBHAcjUqtHhmcH2D0gZHY                        y08BeDDO9ZDD#ZD0OOZDOCZb884Z888B8e0C8  ;  kG:   bB8B8kr;;V :w h;y6988OjeGh8B88888B8B8B8B888B                                                                   
                                                     688BeBBBeBB8B8e888BeB888B8B8B8B8B888B8B8B888B8B888B8B8B8B8MUmAALvyyrVVyAyVlyyrTkUkNmmDe8B8688e,rrN98B86;   :MZ0;  :jB8B8B8BU#8BDC8B8wvv ODXHO6kccmX4jcAtcpmjbqZGe#y               :.vkC8BBeZDO#Z#@ZO#Z@Oq0qOGDO8BVB8B8etHmc8w  ; N6; ;Yje8X;     U0l.  .w8OCg# ;j;,8B8B8B8Bem8H888B#;                                                                  
                                                    B888BBeeeBB8B8e8B8B8O;O8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B888BC;AYrVyyyyrLAyrLyywwkcNhjq8B9vN0pC@yVMcB8j k8B     ;yqO.  8B88eB8v     .HB8Bbk.v@qc@8NNjtcktjHctbkttbZ6qBe6OO2cjjmNcb#9g8B8Be9O@Z#9#q#9ZZZGO9CODOO9e8BevbmUvNtpHBB  v Yer;qg8G8   ,  OB: , ..y8BCbZBy ;; B8B8B8B8BZeAB8B8gj;ON                                                               
                                                   B89BBeg8e8BBB8B8B8B8B84 e8B8B888B8B888B888B8B888B8B8B8B8B8B8e;rNLryrrAVyyYVhXXTkUHHCB8q     w2XwpeBbHjA;88B8y     :mvvB8,   vj;q9mkl;vrjq8O9Z8BO98kAmqh2jHmcpHHmpZbmcpp#9GeBB898#ZeO@pOBO0#OCG99O9OOq@OOOZ0GOeB8BH   L;vqZ0tBN ::;9ZMp@Zj8y  v.,BX     ;H8emm0B8; .Lg8GqO8B8B8O9U88888B8q8V                                                              
                                                  B8B8eeeBe8B8B8B8B8BBB8B8B O8B8B8erB8B8B8B888B8B888B8B8B8B8B8B8vlUrrAAXryvlyUkjHNXkC88c         lC8OykpyV;c88B8Bc      .B; vkVAvyB8A       vODAymhU98O2bbtjp0hmtjk#HH#CHp2HkqmjCO0jpOjOO#tD#OD#bqZOCqZD#G09BBB8B8v  . tDv@OZmB8 .Yr2ZklqOmB8  ;;.Ov .. .v#80cjOB8V  yBcMk:  vqe0l6HZ8B8B8B889r                                                             
                                                 886eBBe8BBB8B8B8B8B8B888B86 M8B8B8vkB8B8B8BD;8B888B8B8B888B8B8Bj;XrkUyvvrwUH4jhwjGB8L             cAgtv;. v89ZHqB8GOcL. lv,   ;mjXT6  vCBZUpVAAAjZ6eB888BG9GqtHtpcHccCHqHjD2cjp0cp#OZ#@q0@OOO9DZq9ODp96ZG888gmvy   ; OBvm6O0j82:ArVy6by#00B: ,,;jp  .:r9e#t2qOGU  vtB8v; .,.   V6O;vB8B8B8B8O8GU                                                           
                                               .B8BeeBBBB8B88888B8B8B8B8B8B89 ,8B8B8 8B8B8B8BH.8B8B8B8B8B8B888B8BlvwVYyTccXMMhrcO8Bl             VjbHkv,vVv;tZr,mcm@#GmmCqv      .;vG8B8#Yv4CG98BGw;        vZB6G8G@#9mCc0jqCpCqC2CjC0q@b9GOO9Dq#9Z@GeeBB8B;   M:  y BBjC8B8B8g#B:. X8w;b0Z8  ;v9G:,y@OgeeZO66ecvUkHB@:wvv;lv,.j8mvZcY98B80mBUB8B8U                                                         
                                              ,B8B898eBB8B8B8B8B8B8B8B8B888B8B  8B8Bkl888B8B8BH 888B8B888B8B8B8B8bvyhUUjqXNHkH98B:              jO8BAyr  ;  O0ryHMwrNMkkOB6q8B80v   mB8Z..Z988em,      .;;wv. ,AV;ypZO9Db#ZjOqZ@#p0p#OGO9GeZBB8e8e8eBOZO8w  ,.Vv  yvcBU.,:..vhXCh   .MckCpbGpv;y8evw8B8B8B8B8B8B8BNcN .V.yv   jBOBk0jObT8Cvcbcvj8BDBA                                                       
                                             .B8B86BB8B888B8B8B888B8B8B888B888B  OB8B k8B8B8B8BU OB8B8B8B88888B8B8XYUj4hTmHmG8#.              NgjTUe#X80wv  B8Nkv8O6gOOeOeeGtp0OB8OL:gB8q888U     vlhtc4v          .;;Atj@#mOBB8B8B88888B888GeOe8eO0bjq8;  Y UZ  mmBO     .v       .vcOOqjj6BY  k0  cBkOB888BmU8Zj8CV,. ,v ;;ABlG8e0BAem48B98D  r8BUBG                                                      
                                            .B8B8B8B8B8B8B8B888B8B8B8B8B8B888B8B  HB8# U8B8B8B8B# ;B8B8B8B8B8B8B8B8HrtjHt08Bt               ;tUbgkvrN;; mV.DBB8MvB,v8B888etwy4ylrwHCt0BBeH0t0tUmOB8B8l.    :..vrUHmyvYjUcpZq8eH;,;;,;X9pl...  ..vkjCbC8y  L,HB; qB8#   ;,wX   ,  rGOOO#ctj9B8;  C9, YZOB8B8B88Av8.kBUHU;;UkDeB8B8B8XkDbGtZ808B.;vB8B9e8Oy                                                   
                                            B8B888B8B8888888B8B8B8B8B8B8B8B8B8B8B  vB8  L8B8B8B88k  Z8B888B8B8B8B8B8wrC8B8m                8BbNt8yyXvV. 98t8G8BU;Oq        yB8B8eBOOZOcBcUMv ..  vcDv   ,v;Yp8B8C4v4tHqOOeB8;   ,   vv   :;vkeCrteBOj8c  TvbBOwGD8p  ,:vq   ;  .GeTHC00CZeBO   lBA  e8B8B8B8B88rH8:8B8@kB8B8B8B8B8B8v849BHeGB8vvv,yvC8B8B9v .                                               
                                           B8B8B8B8B8B8B8B888B8B8B8B8B8B888B8B8B8r.v;Be:.L8B888B8Bvw Z8B8B8B8B8B8B8B8eeBV                48GBBkrTvmBt; MZ;B86g8tv68g     ,vv ;UmTNmmOThB8@    ;:;,;;vB8B86e88OXht#BB888BO4Y   wv.LeG:  YcN:.:   ,VmAA2rlemwNCrOB6BC  .vBy  ;.;tCpU0ZCqGB8pv  ,yT.  t8B8B8B8B888B;BUv8B88888B8B8B8B8BGwZ;8#8rXB8:v;y;tOrp888B8ZZB8b8A                                        
                                          B8eBB8B888B888B8B8B8B88888B8B8B8B888B8B8 MvHByv c8B888B8 UA.B888B8B8B8B8B888A               0CB8gUB8v,;lvV.:;8B8BO,#Bj: Z8B8tk,mB8           y@m  kv        L8B8B#rwHr    VObjw:vjZ6HtO2;  ;.,v    ;qmvkt@j6B8eCcHpALLp8T  vBM  . ;8@hcO@#2bk;    ,v;,.tB898B8B8B8B888.mB.8888B8B8B8B8B8B8BwkvB86;  B8,V;vBk .#8ZOZt68BmOBB8                                      
                                         C8B8B8B888B8B8B8B8B8B8B8B8BBB8B8B8B8B8B8Bm q.8B,U,B8B8B8Br:g yB8B8B8B888B8B8B8h            v8BB8ByyCU;vv,;v, MBG88Vb8jLO;XBBBe9b         .      V0j.    ,;     yB9,.    l4yylwUZB8eDkCB#.X;  vv  ;k8OLVqqDg8D;   ,     ;Bc  v8r.v4e8eO9B9UCeY,.vNjO8B8B8B8ByB8B8B8B8B88e 8;UB8B8B8B8B888B8B8em9pBcO   B8c;mVr8D       ,82yvZB8;                                    
                                        ;8B8B8B8B8B8B8B8B8B8B888B8B8c9B8B8B8B8B8B8B ;U;8rAlv88B8B88.pV 8B8B8B8B8B8B8B8B8B8          BDO8gGXHv.v;;:,,;;@A28B#r Uy ke88k 8Bc:  ;kLN2UHmtkXv:lH  ;B@8B8BZT; :vcv  j8GAXj8BqU:      ;Zr vDZ  vZ8U;MOqBB6;   .   ,ybOm:   ,28B698Bqv4Gy.0B8B8BqC8VtB8BwBlr8B;.eB8B888B,Bk,888B8B888B8B8B8B8BU;ZV8BcA8B8Z  cBvU8g;  vt;ZB8g8#                                     
                                        8B8B8B8B8B8B8B8B8B8B8B8B8B8B8v4B8B8B8B8Beq8e jYD8yX,8B8B8B8U:GYv888B8B8B8B8B8B8B8B8BM      k8B88BBtB8Ok;k:,mU;@p8B0C;   ,m9Be 8.,B8B8B8U ,v .       v#8gOHTYLkZG6jG8HC8BcvpB82:     .,  .., vBy ,O89YV0Z88.   ,,:;jZcL;.    vD8OccbB8v .vvgB8B8B888BGy8BvD8B8B8y:y YMB8B8yvBv98B8B8O8B888B888B8GOcX98BBm;ce6hl4AOX  yB6U8B8v                                        
               yThA                    8B8B8B8B888B8B8B8B8B888B8B8B888.vB888B888B;Z8l;qr;Hk:B8B8B888,hevv888B8B8B888B8B888B8B8BL   qB8B8B8BGp9CjB8c9e@B8BlAV     kvkv8; 88B888B8;      :.  vc8ZL:;XCpZC@O8B888B8rqB#;    ;.;;y4V   ..   ;8BmHq68c   ;.ytZjr,      vUB89ccCB8v  HB8B8B888B8B8B8BbO8B8B888G c8VAB88eV8.vB8BUYVyvj8B8B8B8B8;UUlB84vB@LGbBD9B8ct88B8j                                           
                  vkGBBGt,            rB8B8B8B8B8B8B8B8B888B8B8B8B8B8B8 U88B8B8B8B.b8.UCvtvv8B8B8B8B8.X#r:eB888B888B8B8B888B888B88  8B8B8e8B888eN l88B8BU #C:   4v2,ZT B8B8B8B888OTv;:;VHO88#vhcH9O98888ecY, .v,.:GO   ;jeg4.     .   vkHj2Cqte8t ;AH9UV       .M98B9kcmOOw   D88B888B8BB6D98BZw8B8B8N.Mwv8vUHkhvOUBvL8BH DLMZvj8B8B8B8 UBvUyObv8BUbllqkj88B8#,                                             
                       :hGB8B8CV      B8B8B8B8B8B8B888B888B8B888B888B8B8 HB8B8B888B w8 mGH 8B8B8B8B8BB;XCmyTvpB8B8B8B8B8B8B8B888B88  vB8B9Cccprh ,B8B8688GcHmrvb;8VcB,B8B8B8B8p8gcHZg6D9OjMlybZGB8B  .           tOOv r8Y      :   ;pBGHwCppt9B8M;.0B8 ;lwYvykkCDgZpjtjOHv;M68B8Zet,;mOGCk4mt8B8B8B8AtBchUv ,yM8v8r8yw888chCevB;e888B9 w8h Nlc,NB8OwB8AGBBB8.                                               
                            :XeB888qy88B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8Bc MB8B8B8B8B N8.r6r;9B888B888B8vqDjmm;UB8B8B888B8B8B888B8B8m   y8B86ZOevvG8BeB8BgB8e,vBcX;He 88B8c9B8ptG0rqCOe#MXUcOZ0DB88       ;;. ;qe8X  ,;rBC  ;, :VkOBOZALm@CjC8B8:  .B8v ;t88B8B8B69698B8B8B8B8888Cm8c;  ;vqjUq@888B88::8.    ..b;8BL8vl8B888B8v8 888OYv8BvU88jD8B8B8e#BG0eBA                                                 
                                .AD8B8e888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B, w88B88888v mB,vqb:l08B8B8B8UAN;;mO0 YB8B8B8B8B8B8B8B888B9   MBee8B8B4B8B8eGe8e8e8e8B8,OB @8B6.   ZOcbvymHU0O0jOOC@8e#;  .kBgDV.;Ujjm;     VG8BtU6B8B99pyUmcDD#Og88e   ,28v  hB8B8B8B8B8B8B8B8Oc08B2Z8U;LLycb; HBOjbB8B8B8B;Y8v :;. HZvC;8r;Z888B8AOAmBDXhO8@bB888Bq#6#jbDtC98O                                                   
                                    rkUkqG8B8B8B88888B8B8B8B888B888B888B8X,.l88B8B8B8 v;CT;rb4v,rB8B8B; AB8vhBv 8B8B888B888B8B8B8B89   t8BcAeeBB8B888B8B888B8Bm;8OAw888C .;rA8Cm4h. c@hM20HvCO8BOB8B8B8B84         .CB8HVteB8Bw;wctCpCCm0ZZN.   vHv  yg88BB88B888B8eG@8mYUCOUyw A;cD89lhcv8BjtgB88888Bmm ;U ;B9H0b;B8BV;vkV,H:;pGB8B8t0B8ObCDO6Oe6Dt8B8v                                                    
                                      ;TrAwCe8B8B8B8B8B8B8B888B888B8B8B888,vr,m8B8B88H cLrv,vbqpV;vy98OHB8BX,BL.B8B8B8B88888B8B8B8BC   0B8mvLkCBB8B8B8B8B8B8U,c8v;9rV8B8B8;t886ZHjv;.AvMDrvOU;y .BB8B8B8B8B0yVv;r4p8B6cjjOe9MyVbB8GBB8eN;.   vkBBZ#8B8B8BBgBB88888G6BjrL ,v;v;v:rv,Av,yZH.U8qqZ8B8B8Bck8vktCBvc8B 08 yC8Gj@8B8B8B8BBeB9##G99CCtcUwv,Zm                                                      
                                        ;rArNkOB8B8B8B8B888B8B888B8B8B8B8B8:;mv;bB8B8BG;ANCq@hVvU0br:;8B8B8lCOw.888B8B88888B8B8B8B8M   keBCv:,:vXbD@eB8B8B9vMBb,#BVZXM8B8l;CeGeB9c4Uv.  :U8GN2@;hkt98B8GeO8B8B8B88BGO#OGBG8B8B8B8B8B8B8B8B8B888B8B88BeBBB68B8B8B8e9G8U y0tHVV.wLAv;:,rLvrrkOcCcB9;8B8vvjvH2Xrv8B8.8cpB2kHq8BeBGpbcmkhAhyVvv;;;;,:;vv                                                        
                                          ;VvrTkb8B8B8B8B88888B8B8B8B8B8B888M.yp;vB8B8B8cyvv,vt2M:lqDV 9BqvUCc:jB8B8B8B8B8B8B8B8B8B    :egjy;v;. .;yk@GBqkt8B#k8888kyT.vvmcGB8B8B89ZtpUkU2qeB8B88#CG689DDOOO@GO99GZq@OOO9GOeGeeBB8eBB8B8B888B8BeG6GeG6geGBB8B8B8BgOGe0,.lUpl lA,9: vZOb0. Dbtp0ZB8B8B2L9B8B8B8B8;8BcB8B888@bUrVYvvvlvlvv;v;v;;;;;v;vv:                                                      
                                           .rYlyXteB8B8B8B8B8B8B8B8B8B8B8B8B88;vDY.B8B888B8B8B8B8Bmym9H:;Y#qYvXB8B8B8B888B8B8B888BO     O60rv;v;;.    vrCg8eOB8B888Becepv#;vY;. ,l lytCrwvy4pmjC6O688BO#Z#Zq#C#Z@DZtC@q2Z9#ZGGeD698GeeB6BggeGOGGOZOO6BDB8B8B8B8egODG8C,Bt: ,..V4, ;;Avvv;,tCe@Zl8B8B8B8B8B8B888B8B888O2Trvv;vvvvvvYvVlvvvvv;v;;;v;;;vvv                                                     
                                             rXvvN#eG8B8B8B8B8B8B8B8B8B8B888B88@ b; B8B8B888BGUAG8B8vmb2Hhl:Y8B8B8B8B8B8B8B8B8B8B8      :8pT;;;;;;;;:.   ,;lMtC968B8B8B89y   v;rrv;..   mwLyLHDG@ZB86Zq0C#0@qZ0#qOOOCtCb2qpt@eOO6ZqOGe9Zg6ZOZ#qOjCjOqgq8B8B8BeBGDBZO98 VBY;h vv ,crVVA:leBeBGBmBB8B888B8B888B889OtAlvv;;;vvvvlvYlyVVvvvvvv;;;;;v;;;;;;vl.                                                   
                                              ;jyvZB8B888B8B8B888B8B8B8B8B888888b,B; vve8B88:vAy A8BtvOAyU0B8B8B8B8B8B8B8B8B8B8B8,       ZZrv;v;v;;;;;;;:.     .vvyVVAqg8v,rXyrcbNOVU  keUhm9HlvvHOODjq##0@0@q@C#q6e0Upj2j#ptO@@OD6OO@Dq90Z#O@Z00qZ@Oe8B    qB8GGZO@qBr.OvOw.rrmTvkqUAL08BO9#:vB8B8B8B8B8e9@Uwyvlvvvvvlvvvvvvvlvllvvv;;;;;;;;;;;;;;;;;;;lv.                                                 
                                                Acvvrme8B8B8B8B8B8B8B8B888B8B8B8Bw;OHjy j8B;y9Ur. B8Bv9;q888B8B8B8B8B8B8B8B888B8r        L94vvvvv;;v;v;;;;;::;,. .  ., ;X XZbtcXwD2lN y8#CmZv;cmjebDjpb@bqC#C#q0CDDDD@m0Cp0DCOtObbD#Zq0qOCjjtC0qDGp898B      ,Z8GOZZCeg,.9A mO.kB vXvDBCrlZX.Vv8B8BeCCmmTyvvvlvlvyvvvvvvvvvvvvvvvv;;;;:;;;;;;;;;;;;;;v;;;v;v;                                               
                                                ;v;;;,,v2B8B8B8B8B888B8B888B888B8t:#cyA;vBt gLlB8B8BAUO;0B8B8B8B8B8B8B888B8B8B8U         .OmVvvvv;vv;;;;v;v;;;;;;;vvv;v;yj8B89OZ0Uv,V  kgpU;;:Av;HBZpC02@bqCqCqqZ0#0OO0kcCcHg2U#0kbq#2bjGcCCDZ22qZpe8C         vGBG9OOBv v;A;r: ;v4yMMYyv;N .VtBCUNAMMMLryLyryyYVvlvvvvvv;vvv;v;;;;;;:;:;:;;;;;;;;;;;;;;;;;;vv;                                             
                                             .vyvvvlvlvvvVC8B8B888B8B8B8B8B8B8B8BvwZM;B8B8l 9c;6B8Tvm9l;B888B8B8B8B8B8B8B8B888            kjylvvvvvvvvvv;vvvvvvvvvvvvvvv;MYme8B8Bck8;.cUA; leBZBB8e2t020Cqb0CCC@0Z@ODGB#cZ0pbm4ZpCZ#GDbZZqZjbZCOjOO8H            YB8eOBh.v;  ,lUhhhMVVvrhrcZB89yrkUUNXwNAwLAyrVylYvlvvvvvvvvvv;v;;;;;;:;;;:;:;;;;;;;;;;;;;;;:;vv:                                           
                                            vv;,.;;;;;vvAkY;OB8B8B8B8B888B8B8B8B8 jbkv8B88@ybpjVllUpH: 68B8B8B8B8B8B8B888B8k              vb4VvvvvvvvlyYVvlvlvvvVlVYyYVyrll;vVwywOO68B8B8Xve8B8B8BGtjC@@qC@Cq0ZZZ#O9Be8B8eG#ttOCqOcZGt0Zc0Oj0q2@@G8H               lBB8H vrMkkXrVvyAUZ8B8B8BeXMkNAU4hMNMMrALLyLYVlYvlvvvvvvvv;v;;;;;;;;;;;;:;;;;;;;;;;;;;:;;;:;;;v;                                         
                                          ;v;,;vv;;vvv;YyvvwrOB8B888B8B8B8B8B888Bv;Z#v@8BjyO#ryAAyvvvHB888B8B8B8B8B8B88                    tmAvv;vvvvvlyrALryyVyyryArAyywATUhNA44NLUXXUHcOO0Z8B88OUqq@Zqb#q#@O9BB888B8D0U:vB9pCpO@cjDjj@cOjZ9ZHb88Y                  ;B8wNY;;;;vlbB88888BZyvvVrXUTMhAwAMrryLVyVyYVlllvvvvvvvvv;;;;;;;;;;;;:;:;:;:;:;;;;;:;:;:;;;;;vv                                        
                                       .v..,;;;vv;vvv;vvvvhMc0888B8B8B8B8B8B8B8B8B.;CqUyAHGp..NUNc9B8B8B8B888B8B888B8BL                    ljMv;vvvvvvvvvlrAUUUhNw44HUkmccccjcjm2mjttjp2pj2p2ctp#bDD9DBBBB8B8B88ZMv         BBkcCH9cC2DtC0q0tOtB8v                      cB8B898B8gthA;:.,,:;lyrA4h4AAATrAyLyyVylVlVvlvlvlvvvv;;;;;;;;;;:;;;:;:;:;:;:;:;;;;;:;;;;;;;;v;                                      
                                     .;v,,,;;;;;;Yvv;vVylTmmpp68B8B8B8B888B8B8B8B88; vvMLY. ;8B8B8B8B8B8B8B8B8B8B888B8                      tXyvvvvvvvvvvvvvYwkctbbC0#@ZC#Cqb#0#0Z#DqZZOZOZOOOOGOBe8B8B8BetA;                #B9k02pmq#cOcjtkDG8;                         w8B8BekVvvvvvv;vvvvVVAAXwwrMrArryrVyVyYVllllvYllvvvv;v;;;;;;:::;:;:;:;:;:;:;:;:;;;:;;;;;;;;vv:                                    
                                    v;;;:;;,;;;vvvyVvlhwAw2pcp888B8B8B888B8B8B8B8B8B8HlvvyHB8B8B888B8B8B8B8B8B888B8B8r                      AmNvvvvvvvyyMAU4XryYyyAMU4Hcj2#ZOOOOGGege9Bee6BeBBBG9G0                           XeGjcGjcCZ#NDmGB8:                              Ue8@krwyvvvvv;vvlVrrwMMAMrALryyVylylVlYlVvYvvvv;v;;;;:;:;:;:;:;,:,::;,;:;:;;;;;;;;;;;:;;vvv                                   
                                .;;v;;;;.,.:;vvvvVyylrTNrkjctO88B8BBB8B8B8B8B8B8B888B88888B8B8B8B8B8B8B8B8B888B888B8B                        tULllvvvvvVvv;:,,....,:,;;;;vvyywXkHttC@DZOOOO6OO##2e8                            ;BOC2#O40pjOtB8                                  ;OBewrVyYlvvvvvvlrrwrTATrryrVyVVlVvVlVvYYlvv;;;;:;:;:;::,:,:,:::::,::;:;;;;;:;:;;;;;;;;;;l.                                 
                                hvv;v;;::,;vv;vvVlYVLYrLUCZH@e8B8B8O8B8B8B8B8B8B8B8B888B888B8B88888B8B8B888B8B888B8B                         LtLyvvvv;;::,,,,,:;;;;;vvvvvvvvlvVyATUXcmj2ZqO#ZZOO6G8p                             t80OZmZj@O88                                      hOZHLVlYvvvvvvlyyMrAyALryyVyYylVlYvYvVvv;v:;:,,:,::;,:,;:;,:::,;:;:;:;;;:;;;;;;;;;;;;v;v;                                
                                L:;;;vYv;v;,:lllvlyyyAUttq#jB888B8BDB888B8B8B8B8B8B8B8B8B8B888B888B8B8B8B8B888B8B8B:                          pUyYvvvv;;;;;vvvvvvvvvvvvYvVYyyrLAAhh4UHcb0ZqD#OZODq9G                              ;Bqct@HG88                                         ;mZphVlvvvvvllLrALAyryrVyyyVyVVlyvlvv;;;;,:,:,,,:,,,:,:,:,:,;:;:;:;;;;;;;;;;;;;;;;;;v;vv                               
                                ;v::;:;wNv;;;vLYMyyrcmkUjG8B8B8B8B8b8B8B8B8B8B8B8B8B8B8B8B888B8B8B888B8B8B8B8B8B8B.                           yCNVlvvvvvvvvvvvvvvvvvlvlYVVyyrrMrXw4hmHttqb0C#q#C@2q9,                               OB0AjB8                                             vcOcrYylVvVlyyryryryryyVyVyVLVYvv;;::,,,,.,,,,,,:,:,:,:,;:;,;:;:;;;;;;;;;;;;;;;;v;vvvv.                             
                                 lv;;;:,MyyMvlTAVrUb2ZHHO8g8B8B8B8BbB8B8B8B8B8B888B8B888B8B8B8B888B8B8B8B8B8B8B8B                              tcwYvvlvvvvvvvvvvvlvVlVlVYyyryAAMwUUkHccCb#CZqZqqCCCe;                                bB8B9                                                 Mj2HMVylVlyVyVyLryyVyVyVyVyvv;:::,,.:.,.,,:,:,:,:,:,::;:;:;:;:;;;;;;;;;;;;;;;;v;vvvv,                            
                                  Lv;;;;,YkhmMrrwXCBGm9DGDge8B888BGj88888B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B8B8B8;                               .OmMlVvYvlvvvvvvvYvYYVYyVyLrrMAhMX4kkccjjCbqC@qZq#C2G;                                  y.                                                    .42pUMVyyyVLyryArAyryryrlv;:,:,,,:,,,,,:,,,:,:,::::;:;,;:;;;;;;;;;;;;;;;;;;vvv;;;vv;                           
                                   Yv;v;:,vUXyrUceBt  p899GB88B8B8pDB8B888B8B8B8B8B8B8B8B8B888B888B8B8B8B8B8Bv                                  vO4ryylVlYvlvYvYlyYyVyVALMrMANwUXkUHcjjCbqCZ@#0Zq#je,                                                                                           ,cZCHhLTAArwrMAwMwrylv;;,:,:,:.,,:,:,:,;::,:,::;:;:;:;:;;;;;;;;;;;;v;;;;;v;v;v;vv;                          
                                    ;rv;;;.vZcj0GL    8BDGge8B8B8BmB8B8B8B8B8B8B888B8B8B8B888B8B888B8B888B8.                                     UOTMryYVlVYyYyVyVyyrrArMANwXNU4HmcmpjCb@0Z@DZOZDZ#9,                                                                                              ;kqObcU4wXwUhNwTvv;;;;:;:;::,;:;,:,::;:;:;:;:;,;:;:;:;;;;;;;;;;v;;;;;vvv;v;v;vv;                         
                                      vyv;.v@8Gv      B8Z6BBe8B8B0H8B8B8B88888B8B8B8B8B8B888B8B88888B8Bj.                                         m6XLyrVVVyVyyyrryArTATThw4XUUHmcctjC2#qZ#ZZODOZOZ9.                                                                                                  VCg96CCmHMAVv;;;v;;;;;;:;:;;;;;:;:;:;;;:;:;:;:;:;;;;;;;;;;vvv;v;v;v;v;vvv;vl;                        
                                        vvvYT.        ZB8BBeBBBeOpeB8B8B8B8B8B888B8B8B8B8BA vhH4Xyv                                                #9AArrVyyryrLTrMAMATMwhUUkUmHccjjqbqqZ#OODOOOOOO@                                                                                                      .YtOB9Hlv;vvv;;;;;;;;;;;;;;;;;;:;:;:;:;;;:;:;:;;;;;;;;v;v;vvv;v;v;v;vvv;vY;                       
                                                        vGBBZ#tjcGB8B8B8B8B8B8B8B8B888B8p                                                           tGwMyryryrLrLrrMMhAXhUkHHcmcc2tbC#0#qZZ9ZOO9D9gp                                                                                                            Ttvv;vvvv;;;;;;;;;;;;;;;;;;;;;;;:;:;:;:;;;;;;;;;;vvvvvvv;vvvvvvv;vvvY.                      
                                                           vqB8B8B888B8B8B8B8B8B8B8B8BT                                                              UOrwrrrryryryArwMNXUUHHcmccjjCp@0#qDqOZOZ9OO#By                                                                                                              Awvvvvvvvvvvvvv;v;vvv;v;v;;;;;;;;;;;;:;:;;;;;;v;v;vvvvvvvvvvvvvvvvvV                      
                                                               vc8B8B8B8B8B8B8B8B8m,                                                                  y#MAyryryLrArMAwAXhUUHUcmctbbCq#CZ@DZOZOO9DGB                                                                                                                vmrvYvvvvvvvvvvvvvvvvvvvv;v;;;;;;;;:;;;;;;;;;;;;vvvvvvvvvvvvYvlvvvy;                     
                                                                      .:;;v;;.                                                                         cbMAyALALArMATATMXwUUkkccjcjj0Cq#DZZDOZOZO98                                                                                                                  TUlvvlvVlYvlvvvvvvvvvvvv;;;;;;;;;;;;;;;;;;;;;vvvvvvvvlvlvVvvvlvyYY                     
                                                                                                                                                      vYlAXhryVrrArMAMAwwX4kkHkccttjb0CZZO#ODOZDD8t                                                                                                                   vcUyvyvvVYvYvlvlvvvvvvvv;v;;;;;;;;;;;;;;;;;v;vvvvvvvvlvYlVvvvvvvyv                    
                                                                                                                                                    ymHrrYYyhMAyArrrAATMhw44kUmmcmcc0bqqOZDDODOZOB;                                                                                                                     4CTvyYylYYVlYlYvlvvvv;v;v;;;;;;;;;;;;;;;;;v;vvvvvvlllvvvv;;;v;vy                    
                                                                                                                                                 YkHhTywNrvv;lLTAArAAMrwAhwUNHkcHcmjbCCZ#O@OOOZOD8                                                                                                                       ,ctrylyyyVyYyVVllvvvv;;;;;;;;;;;;;;;;;;;vvvvvvvvlvvvv;;;;:;;;;vv                   
                                                                                                                                              vTkMryryLVllyvv;vvrrThMAAMwAXX44Hkmtpj#qZ#ODOZOZO#OB                                                                                                                         .kCULryLyrYVlYvlvvvvvv;v;v;;;v;;;v;v;v;v;vvvvv;;;;:::;;;;;:;;vv                  
                                                                                                                                            lwAVvVVAyyYYvvvvvvv;;vVATwANANhkXHkmmjpC@Z@OZOOOOGZZCGj                                                                                                                          ;bOkXAMrLyylYvvvvvvvv;v;v;v;v;v;v;v;v;vvlv;,:,:::,;;;;;;;;;;vvv                
                                                                                                                                          VhVvVVyVLyyllvvvvvvvYvvvvvyLwAwwU4kHcmtj0bZZDZOZDOOOOCbje:                                                                                                                            Hb#c4rAyLVVllvlvvvvvvvvvvvvvvvvvvvvvv;,...,,;:;;;;;;;;;;;;vvVl.             
                                                                                                                                        vwVlllVAyyyYvvvvvvvvvllVVVvlYLyMAhXH4cmtcbCq#ODODODOZD00C#CO                                                                                                                              .4bZjNrrVVvYlYvlvvvvvvvvvvvvvvvllv,.  ...,,::;;;;;;;:;;;;v;vVy            
                                                                                                                                      ;YVvvvylyVyvlvvvvvvvvvllVvyVyyLyyyArhXUUccj2Z#OZOOOZ9ZOqq0@CqB;                                                                                                                                 MZOHArVvYvvlVvlvvvvvvvvvvvvv;:.   ......,:;;;;;:;;;;;;;;;vyv.         
                                                                                                                                    :Yvv;vvYvVvVllvvvlllvVlVVyyryLyryryrLMANMUkct2C@@Z0ZqZ@qC#C0pCjeL                                                                                                                                    w@ZmhyyyVvYvlvvvvvvvvvvv;..       ..,,:,:,;:;;;;;;;;;;v;vYl        
                                                                                                                               .yAAArvvvlYVvlvlvllYlylVVyyLyrrArwAwMhwNwXhUhXUmmccpt2jbp2pCj0pCjC2j9D                                                                                                                                       LpqjXVyYVlVvlvlvvvv;;.. .         ..,.::;:;;;;;;;;;;;;vvy;      
                                                                                                                              UAvv;;vllVyVvYllvvlYVVVyyryArMrhMhh4NUXUXkUmkmkmHmHcmtttcptptjtbbCbbmZ6                                                                                                                                          lc2kMVyVVvyYYvlv;,,.,.. .       ..,,;;;;;;;;;;;;;;v;vvyv     
                                                                                                                        ,lH0jv;,vvvlyVyYVlYllvvvYlyyryAATMNwXNUXkUHUHUmHmHcctmcmcmtmtctcjtjj2pqCq0j28                                                                                                                                             bpUAArrVyvvvv;;::,,,...       ..,,,,::;;;;;;;;v;vvv;vYv   
                                                                                                                        l;.vLUlv;vvlvvvvvvvvvYlyVrLALTMNhkUHkmHcHcmcmcmcHmmcHtctctmcHcHtmccCj0C@000OB                                                                                                                                              vkwLAVVYvvv;;;;:;,:,,,,..   .   ..,,:;;;;;;;v;v;v;vvllv  
                                                                        ,                                               ,Uv:,v;;v;;;vvvvlvYvyyrrMAAAwTUXkkHkHHcmtctctctmcmcHcmcmcmcmtmtcjjpjCbqC@0#8e                                                                                                                                               lvvvvvvvv;v;;;;;;;;;:,:,,..      .,,::;;;;v;vvvvvvvvlVv 
                                                                       vY                                                XChVvvlvvvYVyVyyArAAhT4NUXkUHkmHtctc2jpj2tCjb2bjCtptjmtmtctcjtb20bqqD@9g8B8                                                                                                                                                v;;v;;;vvv;v;v;;;;;;:::,.,..       .,;;;;;;vvvvvvv;vvvvv
                                                                                                                          ttkVvyyywU4VyrrrTrXwXNk4HcjjCbq@9Z99eGg9G9G96OOOODDZOZOZDZOZO96GBeeB8Bet.                                                                                                                                                 v;;;v;;;v;v;v;;;;;;;;;;::,,....     .,;:;;vvvvllLAv:;::v
                                                                                                                            O8GO9GZGO2C9OgeBB8B8B888B8B8B8B8B8888888B8B8B8B8B888B8B8B8B8B8B8B8B8g:                                                                                                                                                  vv;v;v;v;v;v;v;v;;;;;;;;,:,,,,..     ..:;;;;;;;;vUv:;;;;
                                                                   ;T;                                                    AB8B8B8B8B888B8B888B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B888B8B8B8B8B888B8B8B8M                                                                                                                                                ;v;vv;v;v;v;v;vvvvvv;;;;;:;:;:,.,.      ,,;:;;;,,.;Yyv;.
                                                                  .yYvv.                                                 CGjZ8B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B888B8B8B8B8B8M                                                                                                                                                T;;;v;v;v;v;vvvvvvv;v;;;;;;;;:,,,..     ....,,;:;;;  ,.
                                                                  ,,:;v:                                                 ;eB86eGgGBB8gBB8B8B888B8B888B8B8B8B8B8B8B8B8B8B8B8B8B8B8B888B8B8B8B8B8B8B8B8                                                                                                                                                vcvvvvvvvvvvvlvvvvvvvv;;;;;;;;::,,..                 .;
                                                                                                                         rBbttcpjj2Z#Cpq@O#ZZOO9GgGBeBB8B8B8B8B8B8B888B8B8B8B8B8B8B8B8B888B8B888B8B8B,                                                                                                                                                :cXv;v;vvvvYvlvVVyvvvvvv;;;;;v;;:;,:.,.. ..      .,;vH
                                                                                                                        ;B2Tk4HkmkcHccttbbqC0q#qOZO9gGeeBeBBBB8B8B8B8B8B8B8B8BBB8B8BBB8B888B8B8B8B8B8,                                                                                                                                                  ABOjCttUhAAyVvlvvvvvvvv;;;;;;;;;;;;:;;;:::;;;;;vvYcG
                                                                                                                        HbMmp0HHUcmHmjmtmtcppC2qq##ZZOOG969gGgGeeBeBeBeBeBeBeBB8eBeBeBe888B8B8B8B8B8B                                                                                                                                                 vBB8B8B8B888B8B8889GZCUwrVv;;;,;:;;;;;;v;v;vv;vvvyLHB8
                                                                                                                        ;UHty;yUUUkHpjcctmjcjcjjbjbb0b#qDD9O9OGO696Og96666eGeeeeBeBe8BBeBeBB8B8B888B8                                                                                                                                                h889Z69ee8B8B8B8B8B8B8B8B8B8BBOZjkAVvvvl;;Vwv;vYYr4t68B
                                                                                                                                .vVAyUctj0b0j2tjctc0bqC#0@0ZD99GO9O99GGg96GeGeeegBe8B8B8B8B8B8B8B8B8m                                                                                                                                               .9jmktjCpCC#@ODODOOG9ee8B8B8B8B8B8B888B8B88888B888B8B8B8

*/

/*
, : ,.:;,  ::;,,:.:;;,,,;;;,.::;;,..;:;,, :,;,,..;,,. . ;,, ..;,,...::,   ::: . ;;;. ..:.,   .;.. .,; . ..,;.. ,,,,...:,:,.,,:., .:;;.. ,.;   .:,..   ,.. :.:..  ..,.  .,.: . ..,.   .   VeBOOG6BOeBeg8HHBO6OGe6eGggB6GGeGOGZ2BBGtcHHAmcHMwrwUUyrvceGGGZGOG688B,       .    ;.: ..;:....:;:., ,,:  ,vvvv;;;v
  .:;;....,,,. :,;,, ,,,,.,,:,;;.,.:,,.,.:;,, ..,,.. ;.,., ,:,..  ,,:....:.,.,,,::. ,::.,,.:,,.. ,,,;.,,:,.. .., . . ,..  ;,,.. ;:,:,,.:,,:..,..,  ,,,   . .    ,... ..,.  .  ;.. . ,    kB899eBB8B8eeghCg6e9BG69egeO9Oe6gOGBBOmyVlyVwrh4tctcNVyyVTGg9G6OGG9e8BBr        ,.,..  .......,,; ..::,...;l;;;;;;;
,.;,...,,;; ..;,;.,:;,,, .,,:.. ,,;.. ,::, . ,,., . ;,:.,,;;.. ,.;, ..;,: . ::,. . ,... ,.;., ,::,. .;;;, ..;., ,.,:.  . ,.. .,,,   ;.,,,..;;,..,,:,, ....   . ,;   , .    ..,   ,,.    .c8B8B8g#twVYXypeeBGeO9O69G9G99OG6GO8qXvVyrUUhAVNNXHOqmAryM68DGGGO9OGOeB8N      ,.   .,,,,  .., , .::.. .:,;;:;;v;v:
: :. ,.;,:.,,v,. .;;;;...;:: ..:,;:..,:. . .,; ..,......;::...,v:;..,;:,,.:.;,.. ,,; . ;,,  ..:,.  .,;.  ,.;. .,,,.,. .::; , ; :   ,:,,  .:..:..., , .,.. . ,;., . ,....  ,    .,: . , . j88Bemv:;vv; U8eGGee99e9GeBGeGOOeD694lAAwkHmtcwrTrXj89cMyvgeGOOOGge9OOGe8BL        ..,, ..:., ...:,. ..:..:;;;vvv;;
, , .,:., .,;: :,;,. .. ;., ..:..  ;::, ,,;,: .,,.,.   ;,.  ,,:,, ;::...,;;;. .,;,,.,.;,.. ,::.. .,:;   ,,:....,,,..,::.; ;;;,...:.,. . :.: . :.:. ..... .,.; : .,;. . :,.   .,., . ,..  v89L;;ycHOU M8gGG6GGGg6gG66G9G9B69BBHLLXrAArACmTyAhmO8D4ylC89OD9O99g9GOOZ8Bem     ..   ..;., ,;::  ..:,: .;y;;:v;;;
 . ;,:  .:;.....;; , :.;.,..;::.  .;., ,,;,,...,:....:,, . ;., ..,:,,.,,,;...:.;....;::..,;,,. ,.;:, ..;:.  ,...   ,.:.,.,,...:,:;,..:,,,. .,,:   .,,. . ;.;   :;,.  .,; , ,.;..  ,..     ;vvlkqcqj c8eeGgGeO696eeG9O69GeBB8eCkAATywVMmmhTATwG6BqAvkBBO9OGOGGeZ9Gj9G88Bjv;,.   .:,... ;.,. .:; . ..;v;:;vv;;
;.:,:, ,,,; ..,,:.,.;,:, .;,,,..,.: ,.,....,.;,,. ,.:., ..;:,. ,,;...:;,; . ;:;,..,,:,  ,,;.. ;:;,.,.,;,  .,. . , ,,,  ,,:,...;;;...;,;.....,...:.,.  :,,,  .,.,.. ,:,: . ,,,....,..    .;LYHtOZjc T8eGG99gGe6e9GOeG6999B9#c4HmkMLMLMcHMhmMyTZBOBmvve9GeeOGG6OZDge699B8brvyVlM;;   ..;. ..;.:.,;;. ,;:;;v;:;
;...,.;,.. ;,; ,..;;:,,.;;,. ,.;., ..:, . ,,;;  .,::...;,,,..,;,,,.,,;.; ;;;:,. ;::,.. ;.. . ;,:. ,,;;.,,;,:   ..,....,,, ...;., ,,;;...,., ...::: ,  ,: ...,,,. ,,;..   :.   ..;,   . .VtCOqZ0OOv:8Bg9G9eGgGBeeGeO66Gge2MVrlLMcmccjcmhrArAMkZGe9ejYweZe9GGe9gO9OeGe6B8Gr;   :l;vvyvvv:vvYv:    :...v;;v:;,v
. .,:,; . .,;,.,:;:,....,.. ,.;,...:;,. .,:.    ;,.  .;:,:..::;.:.;:;. ,,,....::;., ..;.,..,,;.   ,,; ..;,:...:;..,.:;;.. ,.:.  .;,,..,,;.. ,.;., .,:;,. .,:. ,,; ,. ..:,,  ..:     ,  NgD69G9O96.CB6OGeG96Gg9GGeG96G96jMYVyArLrMk9BGUrywAThkp6#OGB#h0B9e6GO99OOOOGOe99Ov                       ,...V;;:;;v;
 .,;..  .,,.: ,,,, ..;., ..,:,..,,;.,. :.;.. ,.,.  .:;,  , ;.:.,,;,,..,:;. ,,;,; ..::., ,.;;.. :,; . ... ...,,,..:,,:: ..;:,,..,::..,,,; . ,,,..,.:.,...:;.     .   .,:. . ,,:,. ...  b8GBZO99e80vO8GGGe9G66Ge9eee6GOBDXlyrANcNhAr@8B#LyrhMArcO9OgOgeeeG6G9eGGOgOGgeZ@Ay.                       ;,.vv:;;vvv;
;.;, ..;,:.  .:., ,.;,   ,:;.. ,,,, ..,.. .,,:,. .:;,., ..;....,:,,. ,,,:...,,...,,;....,:: ,.:., . :;,, ....   ,.:..  .:,.,.:;;: ,.;,, ..,. . ..,,..   .                  :, . .,   c8ge966O688BveBegeGGOgG66G9G9GO99#hvVALMhcMLyCB8gCAVAhrTk@B6669eee69O6eB6eeeeBBtlyv;.                      v,, ;;vvv;;;
:,,.::,,,..;.:.. .;:,.  :,,....:.:...::...,,,,. .;:;...,::....,:.,.. ::,,;,,,..,,,::.,., ; ..,:.. ,,;;.. :.. .  .,....,. . . ;,:...,.:..:...   . .     .. ;Xcp9qkHyV.      ..   ..  U8BeGG98BGHwv.lUt@e6GeBBg6OO9GO6O9e94rlyAhHwLytB9e9#wVyALhXCg8g9GegeOee6GeG9OGBGkrv:                        v.. ;v;v;;:v
, ..,:: . ,:: .,.;:. ...:.  ..:,,..::,,. ,, . , ;.: ..,,;,.,::;.;,;;;,  .:,,   ,v.. ..:,. .,,,    :.... :,...,,;..  ,...  ..,.. .:,.    .    ,v;:.vAVvtOOOBB8B888B8Be90jjl      .  .8BBg88GDHUcp#e6C0tCjbO8BBeG9OZ9O99BBeO2cCkMyTrc6eGBBGCkyVvyVje8BBe6GeGBG99eD6BeHvv:                         v ..l;;::,;;
 ..;.,...,:,..,:;;  .:,,.. .,,. ..,;., ,.;.   ,.,.  .;:..:.:,:..,,;::,..::,:.:,; , ..,:...,,.   .;.   .,:,.,:,:: ..:,,,. ,:,:   :.        ;lLkD8B8B8B8B8B8B8Gg9eGOOeGBB8BeGeH4l    OB8g8GO2ZZ8B8BeB8GBe90mmjB88Be9DeOD#9e8B880YMAAAqB6Og98g9jhVYvM#BBB9G66Oe9G99BBOHV,                         ,,. .Y;,:;;;;
;.;.. .:::. ,..., ..:,,...;:.. ,:.: . ,,.. .,,;.  .,, , ..,,,....,. . ;,.,,.,,::..,;;. .,.:   .,:..  ,,,  ..,., ,,:,,..,:;,,,.:..      ;jgB8888B8B8B8G8ZGeBe6GeG9OG68eeg8B8B8B86X ;B8e8pUDgZO9GZO9GDOZ99BBBtrlB8G6GOeG9OG9GG8GAvAyLNOBgG6OGGBe6pXvV@8ee6eeO9GGOGBeB4:                          :....;;:;;;;;
; . .,::.. :,;.  .,:.   ,,,, ..;., ,.:,.  ,:.: ..;.; . ,:,.  .:,,. ,.,.,.:.,.., ;:,,,. ;.;   ;:,.  .;,.. .., . ..,, . ,.,: . ;     ,vvb8B8B8gBB6e868BD@Z6OZeGeG6GBB8GeGeBBGBeee888O8eB@yb8GG9GOD#G9e6gOeO998e4:H8BOGO6OG#09eOe9tUYvywC9B9GOGGBB8g2yUGB9e6BOg9OGeb9mY,                          v:. .:;;v;;;;
. ..;:;.,.::,.......  ..;   ,.:,:. ::,.. .:.,  .:,,  ,,:.  .:,;., ,::.  ,,,: .,.;;: ,.::,..:;;,  :,,;.. ;:..   :.,.  ,. . ...  ,,rqB8B8BBB8B8B8B8g888B8B8B888BBeeOGtHm#GOO99gO8Bee86gg90GDG9eeg9BB8B8B8BB9GO9BGvve89gO9009GOOZg66pwyvvN#gG9OGGee8BZrDB66egeGgeBOkv.                            y;., v;;;:;;;
 .:;., ..;;....,;,.. ;,,    .,: . ,:.. ..;..  ..,   .,., ,.;,:..,;;,. .::,.. ,,;.. ,:,, , :;:... ;., :.:;:..,::;   :.:    .   pOO8B8B8B8B8eGHXM;,  ,;     .,,:vHmTcct99eG99GOOeGZ9Geg966Z0@9qO88B8b;;vMeB899OeG8UAj8O6@9ZDCOOO9GB8eeC2wm#G96G699OeeZOGOgGe6BegBOHv                            v; ...vv;::;;v
: ;.., ,,,..,.,.,...;,,. ,:, .,.,,:.,.,,,. . . .    : . , ;,.. ,::;...,,, , ;,:. . ,:.  ;:;,, ,,;... ;.:. ..;,; , ,,..      y08B8B8B888Cr                      ;yvw6e99ZOOeO9OO2Zg9DZCGD9BGZZArv,       hB8ee9668evyB9GZGGGZGD9OgggeBBB6G9ee69GOG9eGeGeGegBBZHyV:                             l  ..,;,:.v;;;
; , .,,;.  ,,;.. ..:,.. ,.., ,.;.: ..,;,. .,., . ...   .:,,   .,,, . :,; .,:;:,. :;, . :.;.. ,;::...:, . ..;.   ..,.   ;vh;O88B8B8Oh;                     lUDOZB6DGObZGGGGgee9OZDeeeODDZC9O99Gw      .   HBBGG9BeeBl;8BO9GGGDOOGOGO9OGG69eg6Ge6GOGG6GeGegBeBDy.,                             ;:,.;  .;,;;;;;
, ...,; . ::,, . ;.... .:     ;., .;,:. ..:.. ,....  ..:..  ,,, . :.,. . ,:,....,,: ..:..  ,.::.. :.,   ;,..   ,     v6B8B8B8B6C,             .AtCmtlvme8B8B8BeGe9996Gge89OO9OGeBO9eeOeg9ZO6G68D,   .     G8gOO9B9eB4;eBeG6OGge66OegG9eO9OGO9O6Ge66GBG9BB6Zkr;v,                             ; .;.. ;;;;;:;;
  .:,..,.::; .,,.,   :., . .. . ..:...  .:,  .,,,  .,,,,.. ;,; ..,:.....,,.. :,; . ..,, ..,;.. ,.;., ..:;   .:.  ;;vVDZOO8Zpttlv;V.      .vmZ88888B8B8B8B8gB9ODO@9OOO9D9699eDB699e6Beeee9OZOZGg8B.   .   vBeG9Oe99O98A 8B99eGeG6OOZ9GgG9OGOG9eGe6eG6GeGgOOkl;,                              ;:.,  .,;v,;;;;v
:.;....;,,...,;,. . ,.  ..,;.  .:.,   ,.. .  .,.  .,, . ;.;....;,,,..,;,,  .;:;.. :,.. .:,, . :,:,..,:,. .:,hUOj9B8eZT@jXr ,Mb#Z888tUk0bpcC88B66e9Be86eeeeB98DG9OD99OOG#99eee999e6eOZO9OBG6OOO668#      y88eGGGgGOD96Bv 8Bg9D9eGOOOO66GZ6gG9GOGGBGeee6e6#k;v;.                             ,;.   .::;;;;v;;;
,.. :,;,.. ;,: .....,  ..,.,   ;.....,..  .,,,., .., . ,.., ...;..,,.;.: ..,,...,:.... :.;.,.,,:;  .:, .,:;rA#98B898B8ebA2#8B88eeeB8ee98BBeegBDGDeOeOgGGG99OeG699GOB9eOG999GO66e9OG9DOD9DGOg96ZbB8t    bB8GGZ9OG99q99eB.:8eOO999D96GOe#OOG99OG9eOGGegOqO#wy;                               y ,  .; ..;:;;;:;
  ..:,, ...    .: .. ...,,. ;   ..,,,: ..:;,. :.; .   .,    .., . :.,.  ,::: ,,:... :,v.. ,;;,, ,.;., ..vrtOGD9eegeeBZG9eB8BeBgee6OOOe6OgOGOO99OG9e66e9CZDGZGOGO6eOD9OO9eGGOqq6OG969BO99OOOZe69cOe8, v8B8GGOG@ZOB9GGO9BD bB9eO6O99G6G9GOGD6OGOGG96GeO0pmyl.                               v. .,.:.  ;;;;::;;
 .:;,, ,.:.   . .  ..,,; . ..    ..,,  :,: . ,;:: ..,,.  .,, . ..,,.  ...,  ,:,: , :,;. ,,;,,..:;;.,..;vq8eGGee6GG@#qGODOeZO9699#0D6DG6e9G9gGGO96GeeegGGOe6OZeOe9g99OOOgGDOBOG@ZD9G9O9OGZOOO9GeGbOBDv8B86BG99GDGeODGDO98c;BeeeO99e9GOGGG96eB996eZDO#MTvv.                                v.. ::,   .v;;;,;;v
; , .,,,..  ..,  ,:.;.,.,:,.   .... . ,..  .,,.  ,.. .  ,., . ,;..,,.,,...,.;., ,,;,.. .:.,...:;:. .v;rB8BejOG6eeZDCZDOGeODG6eet#OOOgO9GeO6GeO9Gegeee6eee9GGgO9e6Zg6GZ9OgCGOeZZCC@99eOO9eDZO6D999ZB8. .98B8OOO9OGODZ9O998.TBB9OOOO699O9DO9g9eeGDOjbMrv;                                 ,.,,;,.  ;,,,;;:v;v;
,  .,.:   .... . ;,:.. ,,.,.  :,,. :.,.. ,,.,. .,,.   .;,. . ., ..,;,..  ,,; ,.,,.:..,;;., :,;,,  :vl;tGGCcjCGeeC@Z@Z9e9eOO696jpH@89GGZeZq#O96G9Og9eOeGee9DeGOZO9ZOG9DOOOOO99OOgOCbOGeG9O99GOGZ69OGGH   vOB8Be6e99OG9G96Bp HBe9GO9OG9G9G9eOOOGjhm4yY:                                  .;,,..  .:,  ,;;;;::;
  .:,.. ,,:.. ,,;  . ,,.  .., ,...:,,..,:;. . .., . ..,.   ,.,. ..,.. ..:,,. ,.:.  .;.;.,,;.:..  ;vl.kg99##G698Bc2j0g6ge69Gee#pZqOgDOG96@pOOGOBO9O6GO6OO9Oe99O9GGDOOgOO99G6#ODe9G9O@Z98e96e99O996OOD9v    ;68Be9eOG99D69eB, 8eeGGO9O9gggeOgqtAVvv                                      v,:  ..,:,  .;;v:;,;v
; :, ..:.,...::,. ,.:.   ..,   ,,,, .:.,, ...,.. . ,... ..,. . ;.. ....,   .,:.  ..;., ..:,.: ::,;;.;9DB9ZO896B9hq#DOB6Be998#pCGGBO9OGB9OOO9D6Oe669GgBOBO9G6G9DGOGG699#G9Gg9999e9BOOqZOeeeDOZ9G9eG6OOGv     M88G9G9ODOZ9O8U g896GgeeB8Z0q9ZV;v:v:                                     v, . .,, , ,,:;;:;;vv;
: , ..: .  ,:.,. ,:,, ..; ,   ;:,.  ,:, . :.:   ....   ,:.. . ,.. ;,;;....,..   :.;.   ,,    ,,,,:; t2ee9OBGOG8by6ZGege0CZegmc0Z6OGGO99D@qCZGOG969G96G9eG6GD9OgOG99Og9ZO696DGD9eG69Dq2ZOBGZOGO99gee6OOqv.    v8BG69Z9O9OOe8 vBBge6BO#q0CDkl;.;,,                                    ;v. . . . . ,,..:;;v;;;;
  .:... ....   ..; , :.;,  .,.;:..,;,, ..:,. ...:.  ...: . : .. ..,,.. ,,,:; :.;..  :.,,.  .,; :;,.vpCgGDB99OeBw;BOeG6O9ZOOBmpZZ96G6#9G60#cGO0GGDOOB999eGGOG9G969GOGOGOeOG99G6OeO6O6OZp0e6Dg99O9OGOB90Djv:.   v8BB6GO6OGG98; 8BBBg6mlrTl;. .                                       :;:...:,...:,;,..;;;v.;:;
, ,,.,.::.. . : .   ,:,, . ,, ....:.,..,;..  ;;,  ..:,, ..., . ..,    ,.: . ,:,,,. ,,...,,;.: .;v;vA29eO9GBegg8;yeeGG6Z6eGeB#0g9e6ggZ6eG4kC9GZGZ6ee9eO9GB6GZZO66eG69gZcZ66egeO9O99O96O#CO69DOOgZ99OOGDO#k:vv;   cB8BGO996O8LvOkmUUUwAv.                                           ; . ,.:. ..,,,, :,;;:,;:;;
; , ,,;;.. ,,;. ..,., . .,.. ..,,: ..,:,. ...:.  :,;.   ,,:..,.:., . ;,. ..,,.....::.. ,,;: ..:;;;vr2tO96GgGgGOvkg99GeOeO6690q99DGe8OUC8Ykt86O9OeB9ZOODBg69ZC9G9OG6eg9jB996e699GOB9OZZD9#BGZDG9OOG9GOe9#j4ylv,   lCqG6OG99OjvyvY;,                                               ;. ..,    ,,. . ,.,,;:,;;;;
. ..;,,   . ,...,:,., ,.: ,  .;..  .,:, .., ,   ....  .:..  ,,;,. ,.:: ..,,;. ..;., ,...,, ...;;; vbHNBBBGe9e6Clmee9eOGOGGOO9jGGgg6BO;UeUkceCLBZeGGDO9OeBGGH0Oe9GGeeeOGeOZOegZG9eG69GOq#9O6Gg96GOGB999ee#22y;;v;v TrkXc0GDZ@lw;.. .                                            .;. .:,.   . ,   .,..,;;;;;;;
   :,.  .,,.. .,,, . ;,;..,,:,;  ,,. . ,.;..    ..  ..., . :..,,,,::, .,;;,. ,., .. ;;;. ..,,;.. vMwbB6B6BeeG6cj0eGee6OBO699OZGe9e@GbVTkThjOvvZO69GDOGOeeeXmOG9B99gOOO96ODZ9OeD9DOO99GD@pqOe6eO9ZG99Oegg9#OOcX;;: ,NyvvAHUy4:                                                 ,,...;   ,...  ,..,   ;;;;,::v
, ;..  ,......,.. ...:,.,,,,.  ,,,;.. ,;,. ..,,   ..,..  .;,,. .,.. , ;.;...,,,. ..:.:, ..; ;.  lptZBBeG6geOeZ2qG9eeeCeBtZBeOjBeGGte0XCcYV6MYrv2beBm0yjO8CrXCGeOZGe99ZG6gOeGOGBGeOgO9O9##@GOGOOOOGGOOOGDO99DB9DcLY,yrHbjkyvV;                                                .  . .   .,..   .,,. ,,v;;,;;v;
:....;;;  .:., ..,,. . .:,     , , ,,;.. . .:, ..:.: . :;;, ..,,, .:.;.. ,,,: . ,..: . ;.; ..,;lMC#eee69Ge6GO#DOD8BGt#GBvAG8OZeg96HeHNkN;VkLhVrc9@tUryDegyvHBgZmOBeG@GBBZOZ9OG99gg9eO99GCqOBG9OeGe9OO6GeG9O9DqDeG0crH@jcwHAHy,                                             ,.. . .   ..    .,:.. ,,;:;;;;;;:
  .,:.....:., ..,,.  ,,,. ..:.: ,  ,,.. ..,   . ;... .,:, ..,,, ,.,.:.. .;;.  ..,.. ,.,.. ,;;;vCO9eG66eGe6OOO9O@6G#tH9@w.vB9t@ggB@jkkrV;yww,vVHkcTYlyU@mY;488#pqeeDNcZ0DXOD969BO9GBee9OZOOOe69BegOGOG9OOeDO@DCDqbcbVUUY;;vV;                                              : ,..     ,.    ,..   ::, ;;;;;:;:
. ., . ...  .....   .,..   ;.,..,,,.   ;,.   ,.,.   ...  ..:.. :.;.,..:,:....:,, . ,::, ..;,vA68e8g69OOBGgBgbZAmCHHhhcv  ABOq6g8Ot4vv;;v;;;.;vAw;lv;wkA;.yB8eDe8B6UvArpy:cCeGGg9q9969996O@2BGeG9O9DGDB9GOGOOckZGDcYAAjml:,;                                             .. .,...  ,.   , ; . ,  ..  ;;;;;;:v
v . ..,: .   ,.   . .   .,.,   ,,.... :.. ...;.. .,,,   ,.,,   .., . ::,, . ;,,  ,;;.. ;vVAwBB8gGge99Z#qOe8bjbr0cAYYrv. ;CeeBg8OHylv;,:;. .,vvv;;,VlUl;.rB8CZOc69rYVMLj:vtZeee6OG9g99969OtU#BpG9696O9GB6eBGBc;NTjCC@b#v,,;:                                            . . .   ,,.,   ..,.  .,..  , ;;;.;;;v
.   .,,    ,,. . ; ,  . ,..  ..;    .,;,  ,:.....:,, . ,,,  ..,, .,.:;., :,;,.  .,.; vvjqBG6Z9COe6OeGGZOG9HkGeB9Y  ;:,,.AOG9b90Yyl;;;.,    ;.  ,:,;vv..v8GCchyAbAAyvllv.CjOeG8CO0eG9OGegOZpDeGgGOGO9OOG6OGOGGBG##DZUwDmHmv.                                        :,  ..      , .   ..   ..,.   :.,,;.::::;
  .,,.. ..: . . ,.  ..:,,   :.:   ,... ...:.. ,.; ,  .,:..,.:,, ,.v.,.,. :,:..:;;:v;rrCj4ykwCDOGBB8egGGBGCZBeB8Z   ...:;jqhh#Hvvvv,.. .   ,      .,; ,vcCXM;;;V;;,;:;v:yrVD08cCCDB6eG9BB9ZO@988BBBGeBO9969OGOjUv  .  ;v .                                        vv,  . .   ..,.   .     .: . . :.. :;;;v;;:
 .::.. , ,.. ..,. .. :.. ..:,,. ..:,.  ..; . ....  ...,..,,;:, ,,;:.. .;:. ..;.;.. ;vYV;vvcZ6B6B9GGegeOZZGBeG6Bj   v,,,vhrTN; ..       .      ,   .,.vvTMV  ;       ..Yrv2m89y@yG8Z86G99gZbDb8B8B8BeeeGg6GGG96Dbr;.; vr.                                       v;.   . .    ..,   ,,   ,.,   ...;.. ;;;;:;,;
; ,   ,,.,  .:... ,,.    .:,.. ;;,:.,.,...    .    ,. . , ,., .,.,. ,,;.. . ;,,...;;:;:;LrAAUq2COZBBGZC9e6eeGO8D  ;;:;.;;v,                       . v;vrl      .;..;vMtvUTBevyvcBGOGGe6OeCZc:v   C8BeegeeeOOG9D9CMAV.ywv                                     ::     . .  ,,:...,,.,   .,: ...,.,. ..v;,:;:;;
: . :,;.   ,.;   :... ..::,  ,,:., ,,;,.  :..    ;,. . .... ..,,.  . ;. .:,,,   ;;;:.;:YvyvvVcjpqDbhljGBe6geGBB8  ,,,:;v..   ;                      ,,;;;:VApqCOe8gBDphVypBL,;YeD9ZB#gOD6tpbyT    ;8BGGe#G9BBprwwrr:.;l                                   .;,,   . ..   .. . . :., ..,:.. .,;: , ,.,;;:;;;;:
. .:,.  ..,.. ..,;..  ,,,.. ,,:....,:. ...,.. :.::,. , : ,....: . , .  .,;,..,,,;,,.,,;;v;;HmkHVl:,.TrtU#tGG9O8Bv  .,.;,                             lk08B8BeZOCC#cAvlYN@jH.;.OHO#ge8GOcGhDZZpNmL  v8BBeGqZZUV::,.,.v:v                                 . ,..   .;   .,.,   . .    ..:.  .:.: . ,,. ;v;;;;;;
...; . ,.,;. ...,....:.. . ,,,  ..,,.  :.:   ;.,..  ,,.: ,.;..   .. .  ,:... ,:: ..;....,,;;.: . .;wY;;r0geg9Ge8H  ;:, ,                           .c88B4Uv;;,;,:,;;,vkprv.vlHjkb@e#9GbHbMCO@c;;Hv  c8ee6e0#4H;,.;,v,M;                                . .    ..,.., , ,   ..   . ,    ; ;,  ...,.. ;v,;:;:v
, , . ,;,. ...;.. ;,,,.  .,...  ,,.., ;., ...:,   ..;., :,;;,,.,,: , .  ; , ;.,. .::..  :,;   ..,:vvyycG8eeeeO6B6  ,,     .;vh2ZBe@chv            ,j8qyv,:::, ,,:., ::Al;.,yrmt@OeCZeOpO2jZrLv. vX  :B866eBOtrHml:   ;                          :      . .    ... ..,   ......  : .    ,,,,, ,.,. :.v;;:;;;;
, ,  .:, ..,.. . ,... .,;,.... ;,: ...;.. ,,,... .,;   ..,,..,,,.....;.  ,,;: ,,:;:: . ,.;.  :,,:;lNCGe8e8ggGGG88v  .:;hTCp#j9HbkbcBB8Gr          vcV;;v;... .       .: .,rvyYy#9GLee6TBwZtrkm, vm  .8Beee9eBZmwA,:                      ;             .     ,   . ..   .... . ,.. ...;.  ..,:   :.,;;,;:;;;
. .,:,,   ,,.   ...  ..,: ..:.::..,..,.  .:.,   .,.. .,,.   ,,; :    .  ::;;, ,,;.: . :,.. .,.,;VACO698ee6BGG6ee8k   yCCthAv;;:. .:;;vcmw          ;:,;;:         .,.v;:;;v;V.UMOy0geZbCyZ2@9CT lU. hBBgGOG6gB8gemYv;                   .     .           ...    :    ..,.  ..,,.  ...;   :.. ...;. ;;,;;;v,
.  ;., ..,,. . ,,  . ;;:  .:...  ,:,., ,.,.. ,.,;.  ...... :,. .,    .  :....,,:,,. ;,, ..,,,;lHMbCGB8eeeOZ999GBB8   .,.. ,::,.. ,.: ,;vl;         ;;;,.   A@jc9e8B8BDyA:;,;:hhAVUODgw9UO0HjmkV;yv  e8eBOgZOGe9Cc0BGpA;                            .     ..   .,,:   , ,    ..,   ,, . ..;,.  ,,.,  ;v;;;;,;
v ; ...v,  .,:, . . ,.;;.,.;  ..,.,., :,:  ..,,.  .,:., ;:;:,. ,..   .;   . ;,;..,,;,,..,;v:Ayjmbe8eG6BGZbO6eGeg8BO    ,;;:          ::vvY;       .;;:   C8Cv. 68v;By   ...,vkvr40kgUcCZe;,cUAAXA  N8BBO69Ztpcc9OkcThCpXl..   .                 . .      .  ..... ..:    .,,.   ..; . ..,  ..:,.. , vv;:,;;v
. , .,,,  ...: ,.,.,  ; v,.  ..:,,...:.    .     ,.;.,.,;:...,:,.....:.. .,:;,,.,;:,.; ,.::ywj6888B9pUyACeBBeBeBOBBX   ;::       .,:    ,lMk       ;;;  9B  :8B8Bv c     .:vyAVkCLmjHOUey y6w;yr  y8BGG8O8BGbMUHMHccjclv:.               .     .      .    . .   ..,.  .:.,,   ;... .,;,. .,,.,, ..,;;:;;v;;
  ,,,.. ..; . ..,     ;  v. :,.  .;,.. .,,.   .., . ,,,,.   ,,..:.:.;  ..;., .,,:.. v;v;vLc98B8cTcMy; vOBB8B8e9OD988A   :., ;vpB8GbB8Zt;  ;Y4      .;l.v8v   v@H; .    .,,;vvvMkvltlGp9k ;qprhr  ;88eOOGeDeD##k;;;:...                  ,   . .    .    . :    . ,,.  .,: . ,.,..  .,:. ..:,: ..,;. :;;;;:;:
: ..   :.:.. .,.   .,. , v:.    .,:. . ,:  . ;...  .,,.    :.. ,,:... ,,,. .,,.:.;,;;vvvAhhAAA;;,v...cB8B8#OG90HZBe8Bm   vjOB8BT.;  B8O8G; ;ky     ,:: ;rTV.         ,.v;;;YlwAvvkyZHtj: vkjyr  ;8B8D9#CppkC0Zjyv;                          .    .       .    ... .  , , . ,,., ..:;.,..,;.,.,,...  ,v;;:;:;
; , . ,    :...   :.. ..  V    .,,  . .   ,.:, . .;:.   .   . ..     .. . , ... ::;;...,::. :,lv. Mr9Z0@A. y0kt08eee8BN    :.tk  c8B8B ;8A ;rX      : .            .:,;vvvvywXvV2mcCjmv.V;, ,  v8Be6eDC#c;;;v,   ..    .            .           . .   ..      . . ..,..  .... .,,.: ,,;,;..,.:,.. .,;v;,;;v;
   ,,.    ;,.  :.. , ..v  l . ,      ,   .,..   ::.. :.: .  .,:   ....   ;,.   .:,,   .:.:,;:;.;vlkN0yv, :;;;m98eeGeG8B       ,    Vv  .  :;v:     ..;..    .  .. ,v;;;;lvyyLlVHjtcctM:lv;,,,  rB8GBegZOk,    ..           .            .    . :      .   .... . ,.:  . . . ..,; . . :.  .,,:....:,;,;:;v;;;
  ,,..  ,.,   ,,: ...,.;  Vv..,   : .  .,..   ,.;   ,,:.   ,...   ,.. , :.    .: .  ,.; , :,,,.;vy;;:.:,Y  .OB8Bg6eg8B8   .     ,        ...r:     ;vl;:. ;,:.,,;,;:;::vllvVll4kmTtmVv;v:;;;. ;G8OeOe9g9t,;q22ccjr;                         .             .     .    ..     :,: . ,:,..  :;    .;,, ;:;v;:;;
; .. .,,,,  ,:.  . ...  : :v   .., ...,,.  . :..   ..   ..,.  .,., ...;:   .. ,. ,.: ..,.,, ..,;;;,. ..  .k888eB68gO98O         .        . ;Y.     .;;yy;;:: ,;;;;,:;vvvvlvVVMUTyHkXv;vv;:,. ve89bOBe888ej;;YXhUHjbp                                   .     .,.    .,.   ., ,  ::,..,,;.: ,,,.,... ;v::;;,;
;  .,.; ..::..  ..,.    ; ,; ... . .,:. . ;.,   ,, .  .,,.. , ;   ...,,  ..,, . :.: . .;,,..,:;..   .   me88GDeeBZ2Ge6Z   ,,,;:. . .,     ,:L          Lj,..,;;,,;:vvlv;vvAyrlhTmty;;vww;;;vcBZlmmLckHjcH2UNHcjUj4#BH                          .      .    . ,   . ..   ,.    .:... ..;,: .,;... ,.,;;::v;;:
   ; ..  .;......,   ...:  ;..     :   , ...    :,.. ,,;.. ....   . .    , . .  ..   .::...;,: . ...;;rZBGUyVhGq;kO8690   ;v;;::,;;v:.  ,..;k.        :ZC..;;::;;;vlvV;yLrAYyLwmhy;v;vvUUmcZ#GAvvYVv.  .vMMHrYlr  ;Xl                                     ,.   . ,     ..   .:,.   :.;.. .:;. ..,:...v;;;;;:
, ..   ,  , . :., . .,..:  r.   . , .   ,,   .,., . ,,.  ..;.. . .  . ,,,.  .,..   .,.. ..,..   .vvVyAUNUVVlrVy;wgeqeBh. .v;;.v;yvv,;;;::, ,Hpb;: :XgOCN, ..,.,:v;;;vvLAVVVVLhXAyvVvY;,,LwcTNtqTvvv,;;v ..YrM;;y4ccUv;,                                   .   , ,    ...  ..,.. . :,. .. ;..  ,.:., ;v;;;;;v
:.: ...;,  . ,,  .,,,  .v  h   , .. .,,..    ..    .. . .,:. ..,.,    ..   .,. . ..; .  .:...  .::.;;v:;,,;v; vhHyhCGr;M  ;v;vvlvvv;;;;;,, .;ryLU#B8h      ,., ;;v,;vlVrvYYMLAYrYvyvv;;;ryTylVmZMttbb0UhyXAvvkDHy; .ytM,,,                             .     .      .,   ;.:    ::, .,:;:; . ..,,   ;v;;:;;v
: . ;,; v   .    .:. ..;;  v  .  . ...,...,.. .....,  ....  . ,   ,,,..  .,,,   ,..   ,. . .,.;   . ;.. ,,vvvlv;;v;l;;;y: .vlyvlvvvvv;;.v;;,       :        ....;;;vlllvllyLryyyAAvv;;vvrVVyhjpj2HUrMyrj8OjO82,,89jH#j0lyXwMAtv.,,       .            .      .    ...   .,     ,... ,:; , ..;.. ,.;.,v;;;;v:
. .:.:. .y     ...   ,.y  .v       :.  ..;,, .,.,..  ,,;    ..    ,,.   .,.:   ,, . ..;, ..,.:.. ....  .:;v:.;;v;. ..;v;V ,yryvvlVvyv;::v:;.        .Tj0A      .:;vVvvvVyAyyVMMMrv;;;v,vvLMZpcrLYyVAwtO#UyvvUDM8tv. 4vAccVcGrvyMlrV                .      .    . ,    .     .  ,   :.:.  ,,,.  .,v.. v;v;;::
; ,,.. ..,h   v,:    ,;V  :v      ,    . ,,. .,., . :.,   .,..   ,.   . ,.  ...,   .,.. ..;,.  .., ,  .,.   . ,.. ,v;vwvy .vrvyvTrVvvvlv;:;   ;e8GZhUtCO8BeGDUh,;:;;vvVyAArYywMyYvv,;,;vwjZUyvlyLLkj#th.   4mkv   ,B898BBqOZv .rmYvvvyv     .           .     . .   .,..    .     .     .;:.  ..:.. .;;;;:,v
: .   ,,. .A   ,.:,.. ;l  yv   .., .  ,    .,:   ,.,,. ..,.   .;     ..,    ,.    .:   ..,, . ..:. ..;.,   :,vTvV;;:Vc;  y .vAAyylvvVvlv;;;;,CB8B8ONMUjZeBwXg8c,.,.;vVrvYYVAryvvvlY:.vrtZ2rlyyrTUj0bv:  . .       vcAv ,wO98B8q; :kv;Av,               .           ...   .      ...   ....   ... .. ;;;:;vvv
:   ..: .  ,Yv   v;:. H,  M.  .: . ...,   : ,    . :   .:.. ..: .  .,,   ..,,   ..:.   ,..  ..:..   ;::.:vvL:;;,;;vyH    yv :VYrrvVYMvv;v;;;;yv  NkAAMUAy        ::;vlY;VVTvl;vvvv:,;TbDcVVLTTUr09k:.,  ,;.   ..            ZB8e; V2. ;::                    .          ,      ...  ,,,   ...... .,.,v;;v;;,
  .;... ...  :v.   .vjC   r. ..   .,.. . ,.:   .. .  ..,   . .   ...., . ;..  . ,.. ,.,.   . . ..:,,.   .,..   ;,lrv    :.yv ,;LYVyhvyvlv;;,      :HmcUv;:. :   ;vYlvv;VlAVvvvvv;,,;TZCXYyVXMwkq9r:,, ;;;;.  ,:.              ;ZBVO9cv::;                        ,     .     ..,   ::: . ,,..   ,.   v;;:,.:
: ;:   ,:,, ..;;v    ;;   A,    ..;    ,:. r, .,, . ;,.   ,:.. ... .. :.:.. ..,.   .;,, ..,.:..:.,      :., ,,;;kv    :, .:Vv  ;LrhVVyyrv;:,.   ,,v;;.  .vvv:...v;v;vvYylVv;vvv,. vH6jrvrVhMwU#cy;:,;;v;;:;:,,.  ;.              vTkOtM;; .              .      .            .  . :  ...,;.   , : . .v;,,,,;
; .. ,.,.,.:,v.,.yv.      r    ,..   .,.  :;;:.. ..;.  ..,:, ..,.  . .,.  ,,:... ..: . ,.;;;,      . ,.;.:.;;;MHv  ,:.., . Ahh. ,vlvrYLy;v;l;, .;:.        ..;;;;v;vvvyvvvvVvv..,VkepyyLMMXhkbpv,;:vvV.;;;v;:.:,:,   . .           :OOHt;;;v           .     ,    . .    .     ...,    ..  . :.. . .:v:,.;;;
.   .,. , ;.,, ,,,;vy    .l ,.;.,. ,,.,.  v :...,;:,.,.:,.  ..;.. .,..   .,,. . ;.. . .:;;       ,;;;,;,;;;vvml   :;: ,. vcjyhty  ;vrvrvyvv;;.,,.           :.,,.vvyvv;:vvVv; ;v4M6brLAAMAX4qpy.vvvvv;:;;;;,.,:::., ;,..   .         cecUvv:          .           .     ;,.   .,   Ct.           .::,v;;v;;,
  .:..  .,:.  ,;;, .yN.  ;v ;:, ..,:.. ,.v    ..:   .;,,   .,;  ..:, .   ,..  .,,.  ..vv,    . :.:.;..,.;,vO4    ;:;;;..UtY;vvvwy;   ,vlVYv;:;;v            . .,;vvv;;,;,YV .;Hwrbtrr4rUwNH20N,;Vvv;v:vv;;;;:;,;.,:,:., ...,          y9cLvv.                    ;   ...,   ...:  .y88D0cZCHAyv.     v;;;;:;
, : .  ,,,...,;.: , ;X;  ,y .   ..; . ..;,  .....   ,.. . .,,. ..:.. .:.,   .,:..  ;;v,   , .  .,, . .:;:rpv   :;:,;:.VjHY;;vvv;mB@l     .  :;;:l.         .:;yYyvY:;.;;yY,;Uty;j0MyrrTMNhtjH;vvvlv;v;lv;:;;v,...;;:  :::,,   ,        vOUvvV                        ..    .:.  . v.,v;98BegO98B84;   .:,,:;
; . .,,,.. :,:.  .,..;,   A  . :..  .;,,,  v,,.. .,,    ::: . .:.  . :,, .,,:.  ..:V;   .;,.::v:;;;vvAyhTX   ;;., , .X0Uyhvv;;;#gqjO6by         ;mmAl;::;lY4wmVv;;,...vhAykbr;,VZcLTyrAANHmbL;lv;;:v;;;;:;;;;.:,;:...,,:    .:..        v8Nv;:  .                  .     ..,     .Vrcm  C8BebtktC8B89L   ,:;
,   :..   ,,:. ..;.  v:   4 ..:,.  ,.,:,  v,,  ..,,, .,.... ..; . ...:   ,.:. . ,;v    ;,;;vv;:         ;::vv..,:. hCLLyYrv;;YZGhYLUCCOgGm@O.    ..vV4kkMlAvv...   :vjCmtUU;; ;q0rArwhUNHc#Hv;;;v;v;;;;;;v;,::;;; ,.:.    ., .           NeLhV.                   ..    .      ;,UmkVBC   M0OjcUctpZBBGy  ,,
  .,.,   :: . .,,.   l,   4.... . .. .   :;   ..:,  .,.. . ,,,  . .. .   . . ..lyv. .:;;;, .                :L;; AkCyYYV.,v;v66hvArcUUrO6ggBBGr;                 vXZpjwryv;v,:tBULTXNUrXcDqV:v;vlvv:::;;;;:;;v;v,::;.,, .,,. . .          DCkr;                  ,   ,  .  .;vY  mDOt8;v2v  LOZ0cmUjpmZ89;  
  ..    ,,, . :.,    Y;   m,      ,.;   ;v . .;.. . ;..    ,,.  .,. . . . ,;vAvl,   ;vv         vkHhv,...     N;cjvyhr;.  ;rO#AlyMUUTk9eDmvycDee6XVvv, ..:;v;;LA@c4vv;vVv;:,.YBCwVwMNAMm0gH;;vvvvv;,;;v;;.;;v;:,:::,. ..;,: ,.,,           Ccw:               .      :,   ,#6Bm V98v;  .HBC  cObqmkMmcDZBBb;
...    ... ,.,..  ,..vh   p;   ...v,   Av. . :.. . ,:.. ,...   , ,    ,vyVV:       .:              :492cw;    V6HVrAv, ,.;reDNLVyAhwyqB#h;  :;vhp9qDCt2Cq0UtAXhTAv;yvvvv;:;;,peUyMyAYAcbbq;;;VVY;v;vvv;;;;v;;,,,,;,. ,.;    .:; .  ,       ;9yv ;             .     .  .   mCU00ml    lr ;;Hv;OOZmkkyUmcjOeG
. v   .    . ,.  .,. Vl  vc     vy   :t;  ,.,    ,,;...., . . ,..   vmw:         .y        vTA;      .AmCM..  ;ZwVA.  ;;.v8BGteOC0qpGeBmv.      .vVAVykrhyyVAArLvlvv;;;., ;.v#cAHC0Zg9GB8y.:vll:v;;vv;,;;v;;;,;;l;: ;;:.    ,,    , .       l0cr:                  .  ;   vegO9y    :YVZj4;;m;UqZctjkNttCp#Z
  ;v. .   ..   ..,   m.  kV   ;v.   vc.  ....  . ,          ....  lXl           vY         .,rMkv..,   vGh:,  VOhv. :;;,,eBMvvMlveOrvZBGODpX;          ;... :,..   v   ;;vyvUCCOMAeMv0k;MA,vYv;:;;v;;,.;:;:;;v;v;;;;v;,,.,       ,           Zjtvv;,              .  v           vyA:v;rcAMv: vqZqt0jcj2mbqU
. ;,;   ..,   :.: . ;j   0; vr:    yr  ..:    ,,.   .;vv;v:,    vUv            lv   . , .       Mml.    qj;.:.bUv..;;v.;DU;vAvvLV: .vHHcmcpZe68eOjcvvU;YkHVyHl. ytk;,He8Avrc#b2mA  TX.vNwv;vv;vv;;;:;;v:;,;;vv;;,;;;:,,;;;; ..,...           :2HkXrXv4;v.  : .;.A;;. l           Y8Bq;;    v vyjZC#CCtjbcj@O
. v. ; , . ..,,. ., Vr   8OjY    :r:  ...   ....   vjprVlAMU.  h;     .,      ;;      ,vkjh:.    vkv.,  y9rvVkpv;:;;,.;GZL,:VXyv;MtOZCC2cetrcOctc2BH,yb;w8C;;vyU4TvvhUv;;vc#kYXtmbL ymv,CY;;;YvY;;;;;vv;;;;:;;,:;;:,.:,;,.  :,:,..,;,,        jCkcUAlLVmZ4vH#jtrrVLyUy            k8B8B9b#beOpb2jC2CmUUCO8ee
,  .  ;,   .;: .,,. wv  ;;y    ;vl   .. . ....   ;eBy.,vl;;;l;h      ..,  ..,,r          ;;NHl;   ;cV. .wtU4kkv;;;;,. qkv.Vv ;rpmpkrcXrjbMCZOHykcc2mcvvH, .v;:Y;vHryvvyhmYDrNMvh#jCH;:c0Xv;;lvv;;;v;;;;;;vv:;;;;v::.:,;,.  ,::. ..;,:.   .    UBjAcUkyrvTThyrklvvLUTje.           .O8B8B8B8eZC@tcm2jcX9BeHHy
   ,;  ,. ,... ,,:  Uv       :NA. ., , . . .    A8p   .  :.;cv      .. : ,.; A; .           ;Xv;   4M.:MMv;;wl,;;v,:.O@.k, vV;AOXj@kVNpH  ;H0kvrbtU#0D: .v;;;;;ywy;h4mXNVhqCOGZHUjk2kb,;bl:lvv;;;;v:;;:vv;,;vv;v;,;;;;;  ,.: . :.,, . .,;.     cemycHHXVAATkwXNUXjhmq98k           ;AtkZt0t0#btkMwHH@6eG2kTU
  ...;  .v,   ;..  .B      .Hv,  .; , ....     wOm        ;q;     ... :.,.. vt. , ,vl:: :     jv: .hr.Uhv,:;kvv;v;;,mBr  vp,rk;;qjZpCqZjkhUCANbmj2mL,,yw,..:;MA..v9pChyyATmp; TbhAMUptpLy;vvv;,vv;;,;:;;;:;;lvv;;;;;;,::;;;.. ... . ...         :BCt0mUhktcwHwmNtk4tc vB8BOZ@:         .;Ub0Db@kMrHk9rv0ZHyb
; ,  ,V.  .rv.     L8     :H.  ...    : .  .  VjZv .    ,Lj    . : ..;:,.   yw  :yyHG602Ur.   ty: .HTUkY , .hUvvvv,;m;v0h:rjvvcy;NjXCbyT9tX9900@Tv,vVTA;vM,,.;,Nk:,A989hyC6C.  c2pyycjkGj:;;vvvv;;,;;v;;,,vvv;;;;v:;,:;;:;:.,,. . .:..     .      D2jpcHUjmmcbbHHm2XD    ,;vv,           ;vr4@OZUcHHA, :Uqv4
,   . ;y;   yAv,  ;B: ;   L, ....  ..., . ,. ,kkB      .vy.   ....;::;,. .;.vA, vvmZG, :rCv.,vHAv.TmVAbkvv,  tl;;.vbmmDj@M,:jA.VT;lbcpOUhHc0cmUv,r@r:;U24.,;;vrv.rr..r60q#qmcXXZqyAqOkABA;vvVvv;;:vvv;,;v;v;;;;;v,;,:;;..,::::. ..;,.    ,        ,jbjmtcmj22HVwArL;y;                      VybcVvvylvvvrvVl
   ,   .vy    .4Hr9Z     ;c..,.. ..;     .,  kHem       yy      .;v;;....v: v2v;vhOy    .4H:..NhvyqclXb#Zh:  tr;.;Z8OCHXctpv VX.;pv;wCcCcjctY.,ycy,;UOh: vmD#l vr;;ky :pt@mvATYwHHkcmCC6TvVv;v,;vv;;;,;;v;.;;l;;;::;;;:.,:,:   :,:.. .,.. .  .     ,0hkmHjchMv;;     :                       ;v;Ywc2cUmh;,.;
; .   ., vv       ,      VH. ....,.   ,,,   VOAe:       U      ,:;;.:.; ,:v v@hv4m.   .:. TrrvqUlACHUv;y;   qHv:;28DVyAXpqppk.vcy.XVvyZO#bv ;Aj;.:tpL.,lmgjAkmcv.yk.;hy vqZOVU2UNXCCqv vtvrv;;;v;v:;;.;.:.;;lvv;;v;v.:;;;,, ,.;;:. . ;,.  . ,       MHhkmL;,                                  ,:vymc@mZ0thrY
; ,.,,:.   vcUV.       . YA .. :...  .,.   ,jC4e       vv     .:,:,,.:,, :Ml,yeD.    :.,   OeCmj2OX;. .v. ;0bv;,yOjZrC9U4mX2jcv:ww,v#v;Ur;vcUv vkCv :yCZHwMLrNtZL,,hk;My  yGO9TktcV; :vctyYvvvvv;;;v;: ,.;;vvv;;lv:.;;;v;;.;,:;...;,.,.. ; . .       #HXy;..                                  .,:vy02tHjCOZ9
:  ;... .., .;49O. .     vY . :   .,,. .. .;4;tG:      U,    ;;.;:;;;   .;:XAAy    ,: . ,mec; ;V@Hv.  ;VUC9Av;vVpqpcUVUHTyAwcctl ;jY;M;..UC;,YqqL:;HZqpOUvZCXCpAtjl ;cv,Hr. XmZT;,,VYy;TylvVlv;;vvlv;...:;,;;vv;v;;:l;v,;:;v;;,;;:;,: ,.;,.  ,.      YCvv.                                      hUwbpkc2HHZt
v .; . :.; .   ,rcjwH     T..   ..,..  ,. .Xrlcp       p     ,.;;vv;:.,:,;.VU:   ,;;   ;Ztv. ;ycXw;;vmb@pL;;;:;qkCph.  tcAlUmyLmN; l,  yv, vjUv,v4DbjMrt9ZcD0XV;tDjj, yc,lN; :v;yTl.,rm6Hvvy;vvlvvVlv.,..,:;;l;;;v;v;;;,;;;,;;;v;v.;,.;,, ...,,       X;                                       ,jhNCUUcttq9y
M.   ..:.  .: :.  l9      t;   .,.. ..:.. rtXM#4       m      .v;;::,:;;vv4v   .,;,. ,Ter,:,vtGkmXUrqjylv:;;;;qGCHttttXCOZq2pcbpZCm ,c4  A; ,:vTHUbyllcUm; ;AUMvHOpcCM,.M: Yv. V;:lmb;.XDlvvvvvvYvvAy;  ..;;vv;;vv;;,:;v,:;,;;v;;,:;;v;,..,..         ,;,                                      vY XqcUHmC#kl
.;  .,, . .,:,.: . wc  :   U,.., , ,.,.   pNvlOT      .9    .:;:..,v:;.vAH    ;:.   Y@D#LmHmtbyvr. ;mMv;:;;, 2BOGODeB8eeGeeBBBBeGBB8B8Be#8BObe960D#OZbjmr;;vcmyMLkUmHpCv   ;vy  vZy;,.m89v.vVv;;vvvMMv  .:;:;;vvlv;;;vvv...;;;;:, .;;;:;.;..    .       LO,                                   ,v :CCmkHmqc;;
 V ,... .,::. . :. .B   ;   T.. ....,.   lqclr#v       H,  ..:; ,:;:. Ah,    :..  v0ZTrrrlArvv;.   ccvv;Y;, LBGODO0ZD##@bCGCD#9OOGO#OCZ9896GBZe6eGBOBB9G6e6OOGGOqcHkUUrvv; .  Mr  ,rtbjDgy;vvv..;;vvVl.  ..,.;v;l;;;v;v,;:;;:;,;;:v;; .        . ;,     ;B8hTv                                .;;tOHrAcq8V;:
 U,    :.. . ,,.    O,  ,k. .v; .., ...  MHvYOZ.       NM  . ,.::;,,;mL     ,,  .CDcl;,.,;vvv:    AZvYvv;,.AB#tZDDbO#ZCqjqbCCq@#C#0q#Zb@CqCZOZDO#ZGOZ9O6OG9g9OZ9eBeBe9CgBBHv@v ,;  Hmw;rBtvv;r:. ;,;vyT    ..;:;:Y;v;;:;;v;;,;;vvyv,         .     :yvk88Cy;,,, ,                              ;;MvyrAcG0v.:
 y; ..:.   ..,    , l; rH,     :;   ,., :XVv06w        ,#    .,;,.:c4:    , ,  ;cHy;,,:vvv;;;..  yer;vv:.,l8DkCZZ0@@#CZDGZCCD@ZqO#qCC2ZC0Cbq#2CDZ29###OOZZODO0#@ODZZeDBOBe8B8BBe80vj92mcBy;.:;hv   :vyUA    .  .:l;;.;;vv;;;v;Yvv,        ..        @BeM ;H;vyy.: .                             ;vvh2XkXlv;,
:U,.:,. . ...  ..:   UY;       ,Yr   : .jthOGV,        .ly   ,;, v0w    .:    Nmrv:..:vV;v,;..  ADr;;;;v;vOZ4CqZ#Z#ZqDZD0qC##Z@ZCZ0bjC20pqpqCCbDDqqZ@OZO@@qDZCCOC@C0C20ZDO@OOOeB988Bg8B8Bj;,,.;vL;.    lV      ,;;;::vv;,;,;:vy;       .               ;48BjH@jy  ,y.                          .;lvLvvvvhkA;
vm.  .  ,,;   :.,,  ,mv  ; .;;v;v9j    wHyp4Vr.        : l, ..  AU.   ...   ;DmY;,  ;;Y;;,,,   cblvvlyT. tghjqDqCCZZD@0CZ@OZ@#DZ0#Z2OC0qZDZbOCZCDq#0ZqqCZC@2CCp2b0D##0ZDOZZ@#qODOCZ#@ZG9B6wv,. .VYAv:,..yV     .:.;;v:v,,;;;;Yv       ..:..  .         ;L;   .rZwv;;;;                         , ;yAvlTkqUUv
O9.    , ... .,     .tvvyL:;;;  .yCH; vHVyMr,        ;r, :Av  .kl   .:;.. :mbNv;,. ;vv;,.    ;0OLrVAv;  V9HcZOOOZOZZbZ@D#Z@ZC@ZD#O@@Z9qqqZC#ZZ0q@@Z@#ZZDZ@b02C2CqZ@@0Z#ZCZ##p#D#p#@@0Zq9eeZc;, :;v;;;vyvyrU:     .:v;;:,;;;;VA       ....   ..             ;Apyrtc;.:m:                        ,vMUNykHbpHrk
 vCrv .     ..    . :cwAkvv:   ..  ;4UCvvYv        vvl. . .v;yU.   ;;,.  y9Mv;,. .;v:. .   ;NMy:;      vecpDOOOO#0#@ZqZ0@C#bCbZqZDeGOZOOOqZOZC00@0O#OZZqZZDq0jZbO#Z@Z@Z#Dq9@CC00OO0COZD9eGGXNVv,.:;;;v;;;;rtv     .,.;;;;;vvk,      .:.. ...      ..     ;#9H.:  HBvVpA,                      ;vAvUyTHp2p4AT
    AAUrv:  .,      rU4UV;;v;vvL:,   vAHw        vvv     . .Hy   .,.   k60yv,   ..:,   .:A;;           BCjZ9D9ZO#O@D0qq@2#C#Z#CZ#ONkOe88BeZO#O0##D0@qO0Z0#Z0j@qDZZ0ZZD@#@qDDq#qDCOq#CZZOqOeOMHUv,  ;vv,::;vYXr.      ..;;;;Ay      ;:;.. ,,,   .,. .   .          ,GB242bX                  ,l.AhXkhkmHjmckc
        ;c@vVv;;,;V2Dpwv;;vVwX4AXAvvv:.,;v: .:lvv      .  ;tv ,,, .  vZOMv;.         ,vy;    .        CembDD#@OZD#OqO@9qD@#pCqOG8q. ,;vL@0eg9qZqZ#@b00#qqCO@Z@#qCq@O@q#qD00@Db0CpC@b#qZqZtOgBkvvryvv;:,:,v.;;vUy,      .;;vA.     ,:;.;,;;,.. ,:;     .             r9wy#pk                vhry:;Ujtc4HHccUj
        .r:  .;;VVNwvv,;:vvyAXU4kUt2jpckv;;v .;;;,,   .   MY . rM: .t@Hvv;     .,;vVLl.   .. .   .   yej##0DbZZDDZDZ@#Zq@#OZZZ9BHv,, . . .vkeO@q0q@Z@0q#q@pCbC##0@0O0OC0@Zt0jpbqCCC0qqjjjCbC98m. ;ylwhAkyy;;;vvyLv.      ;vr     ..,.,,:;;   ;:; . ,.                 CjAHby              yv.:vlTmk0Hcc2tbtH
V v.   H;           ::,:vLXXXkUmcmcmtmpjqmpXv:    ,vV.    vr    ;yh0Y VlM:; ,;l;;;,..   . , .  ..    eqbZ#Z##ZOZO#Oq0qCCZ#DOB6Cv  .  :,;..  ZO9b@2ZqCb#CbCCpbbZbCbZCqqZ#ZZC20Cqq@bZ0Dq02@C@jZZBBH    .;:Vymccck4rrlv:    .vv      .:.,,     ,.     ..   . .            Ct4pCX:        ..;L4.:vhU0ktHH4HbjjmA
rvrrV.vy  .     .::;vvvvrwUkhU4tHjUtHmjtHCb0OqY      vv    yA;. rqv.       .,.         ...     ..   c8t0Z#C#ZO0qpO00qqp0@qO8Or. ,...,;;;., ZZZ#Z#CqD#0bbbqjCjbCqCCbZpCbZ#ZZO#Zj0C@qZZZqZCb#bCq@BBBM:       :,   ,yr9G4 .. .,      .;,::. ,,,, . .,,, . ...   ..         HCrOOU;       :;vMVlrjt#tttptkc2kcmh
.  ,4OZ: ,:....:vvvvvlMAMrUhtHjmjHcjtbpttjjj0C9Nv;    ,V.   ;wHyk.       .. ,   ...    ,.     ...  lBmjZO9OZ9qZq0Z@@Zqq2OG6M   ,..;.;:; ,,q88DD#OqO0Z@qC00#0Cq0CqjCZCC#Cq#OZC20C@CZ@DC#Z@qZjC2#0qO8Bepv      :VUZ88GCC  :v:;.     ,.::.. .::. ,,;.: .  ...  .;           ZccC@blk;      ;VvwwcbjCcUHbmtmjpcc
   . ;jv;lyv;,vlvvvlvArhArNcttHHmtmjpCtqjCtjtpj@ml,    ;;          ..    ,..  ....     ,     .     OqcCOqOq@q@OOqq2O@Oe8BN:  . . ,,,.,,.,:;V68eOZ0C#CqbD0Z0#@#j0C2bZCZ0###OD2p#q0q0#@qqZp0bZbCCbCq@@G8BB9OOOb968eOHlMy     ;,     :,;, ..:.: ..:,..  ,,  . ; , .         wOHT4H2OU. .  .. vArUYVMkvkcmmcj0HH
, ,:   #AvV,;vVvvvh4XXHwrU4cmHtHcHcHtcpCCCqbpjCtCyl     ;; ,     ., .  .,.    ..    ...   . .     r6cCDZ#p00OqO@O0OOBB8U; . , :. .    ..; , .VBe9b2##b22p0qC#bCCZ@#CqCq#6Ge6B@#q#C00D@bbCcb0CtjpCjpjCbDGegB9C0p2bAvl8.    .;,    ,:,,.;::;:...:;    ,,: :.,,.  . .        eUHhpUHpmrwvU,v,LcUyvvlhUmUHjc2Cjk
; .   .94X;;vvvVyMkHUkkNhkkmwkHtcctctcC2cpb0HmctCjpH;    r;      .    ,.    .. .   ..   ..,.   . .C@cZZZ0qD#ZD@@OeB8OX.  :,.;,::.  4;  ;. .. ,.MgBOO0ZCbpZCqCqqqCZ0qZZO88e0HmBB89OOO@ZCpp@2Cbbp#CCcb2C2CCZ0##OZ#VlL08j   Yv.;      ,.;;:,:;;;;:, ,.;,: ,,,.    ,.,        ;OCUUmLHmw2HXrpptZ2rrVyAyULhUpjCkX
. , v;rXwyvvlVlLXcXry4tmjkmhkkkUmtjjkttcjHpcpjtHcj0ZX,:  l    .     ...     ,   ....   . ..   ;  vetZOO#D##ZOO9e8Djv    ,;:. .. :XBB6.  ..,:,;  ;D8B60O2@0C@q0q0@bCCOg8X       yGB8BcCO0OpbC0CbcZbC0bcCt2CqqO@mNyyZODBeG9ZHHCv      :;,...:;;. .;.;;., ,,:  ..,            mjHHwccthcccqO0O#C2CUHvXUhAHmcmcY
MhHcjk;vyv;lvllykX4kUHUjkHkHmmmmcmkbmmUtjttccjmjcmc@0L.;,V   .,   ....   , ,    ...    .   ..,.  qDZOGO9Gee8B8G#r:     :,.,.  ;ce8gOBe;  ,;,.,.;..,ZB8GGOZ0Z@ZCqCCpZOB           ,vtO8B8BOZ00jbCC0Cqjjbbq2ZqjchVMHZOjOHlVlVTUGt     .. ,.:,;.. ;;;:,,;,;;:.,:; . .. .      .cpckHmcckmcjjbc2p#Cjccmp0HXjCtjv
MvyvVv;YNvvlYYyy4VhHkHUcmHHHmcmcpkjHjHjjckcckUHHtXkkZbr,rv      .., .   ,,   .,...  .., .       cBBB8888888@mv:,   .:;;:,., vq8BeCOq9g8r  . . ;:,   :Z8BgO9#OZ0b#tq96;;vVO9           :AZB8BeOO9D#OCqCOZqjqDtkVhjZO0tjOVryTyyrpr      :,;.:.;;::,:,;;;;,v:v:;;..,;..        kcjmXUktcmjHtHtcmcttjpjmckmCjHb;
UrXlvMA4HXlrLLryyhrAmNXHmptcpjc#qCttmctjkcmcccmmkAUmbO2,;X,    ,..   ....   .,.. . ,    .      ;tCCpbCcXv.     . ,;;., ,:vveB88BeGOg6Ge8B;   ;:....,  ,mB8eOZZ@Z#ZZeAOB888y               vH98B89OZ00Z@OC22#Urvt06CZjj8trmXylvLOv    ;;;, ,;::,.,;:;:;.::;;:. :;;,.  ..     vemYMkkthhmHmmXckMwHU4wAXmc0v;w,
U4mkUhkkmjmYvvLlvTAlyMNkcMHhjmUkkmtC0mmtcccmpUcXcmkHHmD2vyr   , .  .,,,.  . .               . vV.          ,.   .    .l;;;.rjB8B8B888e8e86 , . ..,:...  ;X8BeCq0#09#49D08B                    lkge8e9qtpZC24Hrp2OOO0HX8#  ;XUAwlp     ...:;;;,;,vvv,:;;;,:,.:,:,,   ,.,      pjcjhmUtkUUtkHhkH4kkLrVjTmtjcY,
vMhHVvv; ;v:;lwMrYlMrmwMTryVvYVvywkp00jjUkUmUHhcUH4NTcc60;v;      ...    .        .,vrkhmUq9e8BU,,..,.:,:;..     ;;N#B4   .  ,   ... .  ,.  . ::...,.:;.  :pe8GOZOZ9@,  .. :,. .                 .UqG9D0mUrUHZ#O9COOcTOB    TcyyT@X    .,;::,;;v;v:,;;v;...;,;.. ,..         ;DpNkAkhkAXN4NXHHUUMVLmjcUc4U;,
         ..;vykhhVyvyHMvvyV;;..:;:;;lyjt2kHUttHUMArr4Mc4qb;vl.  ..           vrHUpGBGD@Cqp96O0B9Oy   ....   ;vT2eB888Z    ;,:   . .  . .. .,;v:  .:,:;,  .. .vDB8e998y     ,v,mpAv;                  ;cqDCbZDZCjqt0ZAvGeUL   vtryMpj.     ;.;v;:,:;vv:;.;;;,;....:;,  .,.     9mUhlUwrATmkUcjw4rvYUmjtcNyv, 
       ...;llMyyvVlv,;v;;;.    .     :vhUbppcHHX4UMUAwrm4CZ;.v;          :rO8e9#bqpjtjtDZe9gq#GXt@pv :.:vhje88B86e9GOBv    . :..,... ..: . ;.UBr  .,,..;;;..   lj8B8BB     ::  vAwvv;                   lUB8B8Oqb#btvre#yck   vcyLyU0t;    ,,,,.;;v;,;:;;v;.,.vv;;,.,,..      :cNUthVUwN4jHmXNNwyhmUtHHhV; ;
    ;.  vvvlyyXAv;vv  ...     .    ::, .;mjcmHpjmMTLwNUmkkjcl.vv     ,;mGBBOqtccbctkmH#t:;vAtBCY4B8B6OBG8B8C068gOOOC6GBeH;: .,,,  .          ;88By   .;;;:, .;.   ;e8Bv ;;r:;   ,vv; ,;v;,.               .,M06B8BClvq8LyrkH   vmyAyyU8Z ..   ::;:;,:,v;;,.:vVy:. .,;          vvrqttcmHjNAr4kHrrXjcjMA;::;:
   : ,,lmmcUYUcjUllMY;.              ;  .vyHNmk2kmwyT4rXwHmq2T  :.;TqeeOOCmmh4MHTHHUUUv      8crHBgBBBg8qOG; vq8B8GZOOD8B8By .   ;.:v:,vv;  :tegg89V   . ...;:....    ,yv;N;vY    .::vv;;;.;.,:               .UkZOjq9rlvyLt;   rTVwXkk    .   ;,,:;vvv:,,vyAv,,,;:,.  .,.     ,vyyUtCpUjtcmckMHjpmMv;;:,::,
v ;;v.mZpccUAUcHpmHNwv;      ;vLwm4.  .   ,VrLyrkjtUXATwkk44tp4; Mg8BBBpcAHt9ctklrUmww,     qBjTjZ9Z9gGHjGBBy   k68B8B8B8B8;  ..:, 28B8B88888B9e86888h    :,  ..;,.   ;N,,vylXyy0   ;:;;;;;.;;;Yv.                ;yUyvlv:vNHvvrOOOqjkA:v.   .    :v;;;,vvkUY,v.    ..,,,        vv:YlrkpjbcUvkrHvv;. ;:;.:,
vv;,  B9kcctUrrtctXky;v;;vj8B86y GH lk:,  .vhhVlAkmcmhAcUUwNUc#pv .H#ZUyrAAv   ;4TAhy4rchyj@b8HhDeODZ9Oj0e6O;.    ::VLUtbOl  :,.. ;888B8B8B8B8B@t8Bee8Bj.     .;.... .;k; vhlVv:Lmm;  :;;;; ;v;v;V;v;:              ;OB8B888B8B8egOOOeB8B888B9jv   .,;:;vM4y.      .:;:....,;.    :l  .;v4XjhvywL:ly; ,;,::;
,,   h8pcjjXTVmccc4MMv;.   MZe8BUv  vUZC.   :YkrhVvVtNrrrLrUtUktcy;.rHwLkv      ,AyvMAmmH4cHCB4rC99OG@OOZCet   ,           .;,,.  ,H@cHh@CbUyvv. ;kB89BB8@v        .,. N;  vy;   v:rVv  .,;;vAv.:vrV;v;. .    , v;; YyMYyUCGe@OqD2Ctbqq0q9669B8B8jv   v;;Tr     . ;:;.,.,:;;:,      ;   .,,.,vAy,vvv; ;:,.,:
: ., CBCh9b4yVrUpcNwhTvyv:        vY,  A6L   ;;MNkAvALVvvlyklMrNktAv.,TkA,      ;rLHXcjmHp2cjBAhb9#99D@OO#G8Y    . ..:. . ,::.  ..             ..  vt6OOO888Dl     v.  Mc  ,wl;.,v .0Zv. ,.;;;..;vv;:lvrTX;v:vrmvv;;v :;;v;XCGe9#Z#2C@jttbbqj0C9G8B8r   ;r     ..;;:. :,;::..;,;     v;    ,;; ... .  ,:;, :
; :  O8U vOcwVvvXttkkMryNHMv:   ,,  ,,:.;;: .v;,;YTNHAA;vvVyylyTUmNyl.vrUYLvl;VUNrT4UAVytpZttgHN#ZZDOCOOOD9O80v         ,,;,,,:.;..    .     .  . . vGO#OGODB8eHl    l. Z.  vAjZpjbVOOCj::h;,:vv:,v,.:v;XMAALvThV,;,  ;;lVyy4m2ZBDe#ZqGgG#@tjtjcqD9889l v      .,;::..,,; . ;;;::.    Y   .   . ,..   ,v;. .
. .  gB@ ,q@cUvv;UCbkyhArrwyrwAMYvvvvyvlrLrLvlrvvl.vvLXXkrv;lVhyYvyXXy;;lUh4Uc4ywHHr.     ;Cte2HCOZDO90C#OqeZ8BDrv;      . . .,::  .:.       .      HBDDO0@2COO8B8p; ;e#em   HZ8G#eeCZe8cUUm;vrwyvvHqCHwrvvv,;;;vLhAM4yyAVMUmkUt@bZZqCpb2pXUUmjmC2@ZO8B8V     ;,,..:., ,   ..:;,..     y., .   ...  ..;;,, ,
     O8GZtpmcTArvvvmptmrwMAvhlyVVvVvvMkN4AwUHvvvArl,.;ywcHMYvvyvAvX4ULY;:yUy  VTTh4Tv      t@92AC9G@ZDqCOZOO#eBB8B8Be9bmV    , .                    w6Z##ppb@#C02OeB6qOO8B;  ;8BGD0@b2GOqGcrbpAYrryvA2C#trVvvyTpbmtmO0OCDCCp#0C2bC#0qpqpckHmmXAwpNmCbDBB     ,;.: ..;..    ,    ,,:    ;v. . .... . , ;::,;;
. .. U8ejHjjcrVXv;,;4OctAUrNyvvvVvYvVyvLV;;vlyYVYvlyr;  ,;ym4Av;vlvyVrrhrl;V.  hlwMUpcVv;;rckjmVcZZOpZZO@Z#996Oe68B8eBe8q        2B9CcptgO@ttbbXv  vOOj0j#CqjpbttZO@COqO#BB   ;8B90O09#ZbDqqqtbZG6GbMLMCUY;vhZO6qC20DZ@Z0eZZZ9q@qO#OD0OZqbjckmycHcrAj#OO     : ,,,,.,  ..,.  . ..:..    ,  ..:.  . ,  .::;::
; ,  v8BDUCZpHwLl;v,vvNb9bkrvrrlyVllAryvv;vvvVrv;vvUmwv   ..vvvVvv;:.:vvv;v;;vyrYvyMhkmmC0thrAOrjq@ZqDOq##geU    ;l#B9ZBc       ,O8eeB8OyO888eeBBOee6qq@C0btCjDDqCOc@@#CO0ee   :8B89O#Zcj00b#UO2vvvMeB89ZmXUH0#0OZGDO#ZZCCDOZjDDZDZC0jbbZk2b@ccUrXAA02Zp.      ,;;.    ;,., , :   ,;,     , .   . .   ,;v::,
.  . :98eq;;DqUmwHVy,  ;rcjcHUkhYyvwlAVyv;vyYvlLyTAAYAyv     .;vv;;v:, , . ;vY,vtAvYLXckkUrULr9Uk#@@090CDOGy       ,eGO8C       .q6ZBb    ;tO8#ODDZODCZOOq0tpjCO0q@#0q#C@0#8e    cB8BGZ0qOtXtCU.  :v yH8e6gG#9O9eODO0Z@ZCC2OZ#Z#q##O0Z2tmt0OZbtHHjq@kpccy      ::... ,      ...  .,;.                 ,:,,:;
: .:  TB8g   ZZjmkrNv;;, .;TC#OcmTwArvlATlYArvyXbA:.;v;Vyv      ..;v;   . ;YMl;:,NUlAAtkXAMrylCmM#90Z@@D9DeV   ... ;BG9Bc   ,..  2eO8g.      c89O@ZC#0#@ZjbpCtCp02ODD#OqOOO@eO     ywBeqq0t4w#j    ;    meOq#DG9eZD@DOOCOODZGDOC#DZ0Zj0Z9OZcbtckbqZCCj0DG,   . .  VL  .  ..     . ,       :UvVyV;yv;;.,.  ;;
. ,.  .OBBt  4D0mUHHAUvv .  .;:vkCOCp0p0bcNkrCt#V ,tGe88BB9G;           .:;: ,;v vUkUXXmwAXhAlvqybDOO9OBB8B8;  : , ;t8GeN  ..,   Og8ec: . .  .9BZObC2DbZbbjC02pcc0Z#DGZOZZ#jjGBv     GB8DO#Ck0O;         4G#2OG9ODOGODZ@99DODOOO#OZjcUCq2HUrVC0CCbHc@bjmm9       ;ADY   .,.    .       .   Y0HYrYrUkhyyV;...
: , .  v8BGCAXjcccmrwlrylvv:.       ,.v::    ...,;vUkvy;v4jjZjv         XNAt0V  ;.rphUrAUTrYv;,jHkCOOegT;U8B8c      cTeBm .     YB8t,  :..    .w6ZODZ0qC#@qpbj0C00ZC@DqZOq#tmTtem     ABBOOOC#Ok      vv  cZ6OOO#Z#GGOZ#eZGO9CZOGB9tAvYlyyHktkbZtjkjbjCUHtBv     ;Ajqhv,.    ..    UM  ;:   HUryyvrVv:vUc;v.
,  .  . 08eOcccc4kU4VlYALyvv:;;;                 CZmy      .hHeBv        vkACey.v HCUk4y       mOm0eZH    ;OB8G; ;U:elA89... ,..v8U     ..    .@9OqOqC#OqOO@C@0Oq@20qZqZ@@jCU4LAmZv    ,BeZOjODy .:8. V   vgBGOOZ90@COZOeOq9DOO0cU,;v;:. .VUm4TTypDOc0tCHqjev   .  yB9Gc    .,k:   yG; ,p    kXrMwvvvLYALyrV
, ...   :O8OcTNHmkmXrrYYyvvvv;;;;;:,              ;wrUHy     ;vV0cH;         tj,vm@mAAAA;     vr0pO@gZ       :qB8Ucplp;B;  .::, ,v  ; .    .vNeeZDZ2ZbZOD2@COZ#qOCCb#@@2ZbpcMVANyhZU   .8OOtZZb: l BC     ;HO9D9ODOZZOO0D##DO##HY..;v     ,,.  ;rNOjjmCt2pcHgv  v   8eGeh     U4    k:  Zw   jkryLVVLyryVvyX
,.:,    v96ecXU0jkU4wXrwvrvVv;::;;v;,  ,            cyv;rM:      ,rkjUT;     HUhjHk4Lyyyy;  ;8v ;BDGGBA         ;2        ,;:;..   ,    ;V0BB86G#O0Zq0b#00bqqCqZ0Z#ZCb2bpmkHX44vXhr8N  v8eC@02XY Ly  v   vVO96OeO9ZODGOZ0q#ODjvLwrlv, . ,;   ,vkA:v#9#ZC0ZpqO8      G9bjBB;    j:   v.  mv  MqwL;yvyVlYLwrYr
; ,   YB80GeOMU:MCtmUThLYyVyVv:;Y;; . ...            A    yUv      vvvLvVvVvUkjHUhrMhvvvLVjeB,   9CZG98V                  hT;   ,;..,mBB8B8GDCG#ODZ@D#Z@Cj@ZO0Cj2C#q@CZCtHjl:vc 286Be   BBO@@jpN  ;vrg  .;DBBDDZDOOqOZO#9OOqqcyvvv;;. ;,;.  vryyly;h@CjbO0@tZBc    k80UUO9:    Uv   l.  l  McrVlvvvVllyAAvry
,   rBB8#p2eek   ,HpkmNMryYrvrvv;:.,,. .              V.   ;YrAtAyyhrl;: .4mkUkmhNLMYv;yVXZw     HtCO9B8U.                ;;  .    .vmC8eeB888B6OmCBBe0ZbZCmk2cpCOZ##DjjUcv:    .8B8B;  28G#2@pG;   rb  ;MZeOGOO@9ZG9OD9O9ZObbcYvl,..;:, .,:;..lLHAwMq0pZ9btcce6O@G8#pXjCOv    Mv   y; ;N;wMl;vylylYvVvYyMVr
  vD8BG204UeeOv   XmkmkkXrY;yywvvlvv;;    .            kUyvvyYLvhTHcUk.  ;tUNTA,  ;vvvAwtq       ;gce88BU                           ;,,  .vyr4U#Ov,yZOBBe@CUU0#2q#bj@jc4Nt; 2     vUBO, .B89D#OeC;     ;rp8eGZZ#OOG@9OOOD0CCpUcUXy;vVy;  .;lr;  rHkUrtCqqD@jctOO89OZmXcC@t;    v;  ;r  lNyXv;.lyM;v;vvlNyvvy
 4B8BZcCCywB68qcVA4U4ck4yXVyUXyvvAA;;: , .             ;ckVrrMMhMUXHmHA,vkXMvA.     rtHcw         68B8Y                                          ,   vykBOZO#OC00ZCqjmUkHmv rv,,      .  ;B8BGOO9BB8U  hkGGOZC0ZCDZ@ZOZ60ZZ0CCjHAhrX4ULTwULl. .VrymhXm2@cZZ0p9D602ZjmNrwbr     w   v  M6B8B8e6BeZbhLLvvyVlry
qO8gZjcqwvO8eeGmtXmmjUHhMH4MyYvVvvv,;;,..               XmckhMhtmkkXHkttjhlvMM       2ZV       .  v6;                            .            .      .  vB6Z90OZO9OqHhhMrhO  v0ZkLV,       4eGGOOOOGejHjG6OO9Z999ZqCD@#0OD9OO@OD@DOC0kC982#hwrv4jNAjcUCj@#ZZqqp0O#CcmMXAO     Y,  ;, O8Be9668BBB888BBcYyhANv
qeGZbpZq;wBBeG#kkkHmcHk4UmjUvAvyv;.;;;,.    .            tHH4mCGbptHMUyhlvvvVXy. ;.;rj      . ;;,:v                  y;          .           .    .,.    GG0OOOOD#pcyhwrh#e8,  .v:cZm,  ::  p8GG99CZ#69gOZ9OZ6ZOOODg0C2bZOOeO9#9##tjt00mjND#b9bch#tbtj0#b02OOqtj962jcyU#e    vY     t8g9O9OGO9ZqZe9eB8BD0vvv
GD@@Z2eXAgeDO0ctt0tcXHcmthjpHLrvv;;lv:;,  .              vmhtM,.;..hMyYvvlvLyrkm#qpbc       ,;vvvv                  e#U        ch;qCV  v;.      . ..  . .#GCZ#D#CULvAh4teO6B8c     r8Hr:vX  eB99G#eppOOODC9O9DZqOODD9Cq0Z9OZD##@O#t#CptmHrqOOZ20C2Z@pqGe9q@#O#CjGDCUUAkTcXywm;.    veBeq9GGO9DD0OC2j#GBB8@l;
0t0CqGG9BeOZZctbjtbkXHHp2h  ktT;,:vv:,.,                  wHL       NAlVrtHcwkXmNyHr       ..v,;y                  OqZ6       gClb 9B8HLB8B9kV   ..     t6OOqGbcrYvMykjDO@96B8Bc    ,8OMvl.  8BODDO69OOOZ9OZ#GDOOGOOOge6CG#GOCq#qZjDb0tZHbHrAcj9DZtZqODGOq@02900@DOOC4vvAp4jHmvvjZcVO86OD9ODqD@#Ojbt2HT9B0kl
9cZ0O9ZeGZeOCcj;  ,mjXHXjv   LUvv;v:..:;.    :            ,kA.    vvtkHkULyyllvv;vv.  ,:;.vvYv;;                   j8@        @BkDj:2e60HCGg8B8q;    lMZgO9ZOCcYvA4kcC#00jGbOD8B8.   .OCYA;  c8O9OGZ9D9eeZGCOOO#DZOO9ZO#GOZO#qOZqDOpqZODZ00cA#9OG2qCGOGZO@OZ0DObp9OCjrvYUMHU;UOqptA.lO8G90O99OZZZCZDOcA08G9Y
9q0206Z9#GeOc#H    XbNXrkU   l;v;;;,,,,,, ...,             Hpmhv .                       .  .                                 v8BOejN9qG0@jDZO6Bek;mBBBO0DDOtHyActtCbZp#C@p00OOBB8v    VUml  yBgGGZOO9O6D9#ZZOOOOOZ9ZGDZOOOZ0D0OOOqC@GO9OZGB9eO#ZOZOGO9O0OOZbCq2#ZjmTwyUkyLlmGHL;;y:YCGeO#Z9OZOZCDC9CcjDGeBH
t0ZCDGeOOOZjttZl  .UcNUXkUX ;vvv;:;;:,  . ..:              ..                                                                 ;B69OGGODC#qZDZqOq6B8BB2qCO#CjLTAt02qODDD#ZZOZZg6Oky0U    #jh  N8G9ZOOODGO9OOOGD9O6OOZ9ZOGO#pqD2@CZO9#OggO9Z9OG9ODOOOD9DOZ@@O0ZbjjpNUMTthTYAvvNOmvv;y;Yc0B9D9#OC0j0jq#XhOC0bGl
MU2qC@CCCbmHUHc2mU2HXhNXrvvyrrvv:;v;:; . ...                                                                                  ;BDOO9Z9CDZ0@@0q2O9OCOCDDZO2UyyktbjO###qqZDOOO9BqL;yw8g   .69  UBeOZG69O#ZDZ99D9DO9GO9OOOG##hM@9q#G99OZGZ9ZOZZOZbZ0DOC96O9O9qCZDqOCyhrrZhvvtvUvU4;;yr;,AjeG6Oqt#mq#2jjmqC0mOgv
cwcXN4mHCCHmpct4tjkMXANArYVyyvLv;,v;;.,.:.                                                                                    UBOOGZq00Z#ZZqDDOD#O@#ZDOD0cUU@bC0#qGZq2p#ZO9OGklvyYt68B.  ,BBkr8GOOeO6DOGGDO#Z#9O9#GOZGeZDjZOqZ@kO6GOOO6O9G9D9@bqCCZC6Gg#Oqp@OZO0qTjHMTCrHtcHrNr;,Vv,.:beGOD#b#Z0cC#Cqc00Dpb;
C2ckHmHqr   NpkUNhAhMrVVLyvvv;;vv;.;,.. ,. :.                                                                                 U8gDOZZ#ZZbZbOCOODCDCOOOq@kbbDCD0ZO0ZbCZCOq#G6hvlvrHCGg8T;. ;#kUgO9e9O9GZGDG#Z#ZO9ZOOg0G2jZbCZ0j0bC86G@@99D9ZD@ZZ92O@9Z9Z#0@Z0DDp@q@bc4N#bbkmHAcCU,;.:,vHe9GD9CZ#bm#CZpqb9Dyh;
,VjtcUtp     XtXUwUrLvArNvv;rv:;:: ::,    ::,                                             :,,                                 .GeD9#O0Z#02@D@Z@C0pOpqb#cbCq2qZO@#tC0OOOpZ9DyvvlltZqZB8k,y; .tZOZDZ9Z96OOGqOO9D9OGG99D#DCbCpU0j#Oc0eG9O9ZO#ZZO@Dq0ZDO0@#0ZqOpZOD0ZZCmmhXmHAkAkmmwv:T:; lDeOD0ZCqmjtqZ#9OeOmH:
   vkHmc.   .UHjUUMNyUAVllv;vv;;:,.;..   .,.                                          ,v.: :;v                                ,ZD6#DZDZq@DZO#Z2##qZ@jpj2qCtjb@@Oj@#9OOOGD2lvvyvcqOe8Bv,  vAlCqZZC#qO0O0DOZZ9ZZZO9G#9OOO@jjkcCC9pHX9BO9ZZZ#Z9DZ@2#0b#CZZZ#6Z#0#qZDOC2mHmkUUTykMANv;T;v.vOGOODO2tjZb0ZOeOZ0CT:
  .rHUmkHvvAtUhkcmhvvMY;vvvvvv;;v;;,, .:;,.                                       : ,.:   v,:r                                kZOOZqZ2D0DCDqDO9OBGO9O#Z##2CC0CC00Z9ZO#OZmvlvlM#OeBeH      y2CpqOZbGOCCZqD0C#ODDZ9OGOOCqjHkCCZD9y;XmjBBG9GOtc0ppCqC@qZ##ZODZDO#Z0qbD##jHwjqbMtvTUU.l,vvAGeODZZZZqDj@OeCmtbr; 
cH2c4Nwk4kUtr4UUv     vvvvvyyv;v;:;.  ::;                                          .... .,;;:A                     ,          ;OBG@Z#ZC9bDDOB8B88GO#pG00qG0Oqq0#DZOOO6phvlYVU#GBB@:  ,,   .jO##GO#ZZb@#qZbjO9OOO#O#D#qtbbDOGZ92L mH.A8BeOG0tc0j#jCj#C0bCOC#GqZCCqZ#Oq2kb2#jUkLALl::,v;L8B6Gg#G99CZ@Z0pMjtv..
ZkmC00CwUrAhUhXc      lvy;v;::;;;....:v.;                  LUAUHNAVyyUrNcpv        .   ,;,,.;;Y           ;      v ;           v99OZZ#q@G9BB8@;     jZ9OO9OqqqZDOqODGklVVYlHCOODv     .   .48GeO60Zqq0ZZ9Z#DZCq#q0Z#@tcjGGgZpV   :y. :8BB99Z222qpb0#@0@pZC0jp2CC2##jjjtCmUHctV;;j..l;.YB888Be0Z2@qjCHyyYyyr.
UmUAvvv4khUkXwrcr    .vV;vv;:,,:   ;.v .                   hUMv,.  vAv;vAv        ..,:.: . ,;vY                 ;v              VDODGDO9e8B,        ;mGGD#q##DO9q96OMvvvrAh@GBy, .    .    ;q2OGO9DO#ZDG@OOOD9qG#OOCbb0DO#rvr,     .  .OBBOOjj2CjqCqCCqCZDcjjCCbjpCpm2jmcjHmtwrACv:4;;;cODCt#Zmlylv;; .vH;;;
hl     wckUAMAMyHchYMYvv;:;,;;;,,..::.                     hyMv     y;,lt          .;,;.,.:;;vv;                4   ,           vqgDO9eB8v         y089O@CbCO@ZDZGbMvvVyXUeeH   ...   ;       ;m9q6ZD#OOqOGO9O6OGOOCqCpCbA  y;     .    AB8@tb0cOCqt#C0qO#jCZqObCmqb#jtcCt#p0mUl4yvlvvy;    v;:;v,, vvvMj:;A
H.    .HAAlllwlvvyVvlv,,:v;v,,:,,;;,  ..                  AUvv;r    v;:y       . ,.:;;,:;;vv;;l;               ;.               lO0OZ9B9           VpB@0pO0ODDZ9DkvlvrVkt9Oy.        .         ZG0#9ZZZ##G#9G8DDZDZ9CqtCbV  :l   .       .B902tttbC2qOpbpmHHCCC0j#c0#CCCZb0bT4vvlvvL;yV.   .v.,:.;vvryvrwlyr
Cyr..vmNVvvYAyVvvLv;;:,.v;;,. ,;,    .:                  ltv;,,;;,., ;A;         .;v;:.;;vv;;;v          .     ,                2DZ@geA          ;D@6@q09Z9Z@ZeqU;vvYycODr  .,. .   .          vGZqgGGegG9ODOVlvOC@9O2jq6v    .:.         qB#2tkCp@2pD#0@mptjcppCctb0p@m0pjArmUVkvvvyjl    ;v;;;;vyMvvv4Myyv
m4kUpjXAYYXXALLVvLv;;;;vv,..,,,;                        .Vv,:,:,.... ;r     .   :,::,:;v,v;;;Y;               :                 vZB88          HB8B6O@DZODODD9jVvvvlAcZM:  ;,v , , .            4GO0DBebZ9vyt ;k9ZpqODpG6                 ee2qjjCq0CbCqDpCHctmkjttHj00HbkHykUCCrXyvlNU;     ::;vvvvY;lYcTwyL
H4mNHMAlVyXyrYrVvvv;;v;;v: .: .                         ;...:,. , . .,;     . ,,;,::;;l;:;;;lM                .                  vTm         ;8BBOD9Z@OZDOOOeqrvVyymej.  .;,,,.,.:.   .,      . :yq;lc;.vUv;CH9GOZCHDDZ#8G               r8@qC@j0p9C0qZpHjHttt#cUHtcZAwkAkmcjptVh4kNA:      ..vvvvLvAVMMyUHm
wrMAyvvyvVvlvvv,..   .       .                             .                   ,...,;;v,:;;;h;                                              V889OD0Zq@#q2ZCZ@UvlVrcbv    ,..  .  .                ;    ,;wmpC6Oq0C02jjt0pOB;            r8#HcbtppHc0jjpmctHbjjpmrUhNUMALTtcbCcUN4UrXv        ....:vVwrlrr;wk
*/

/*
                                                                                                          .,.            ;;;,                  .              ..                                                                                       .v                                       ;           
                                                                                                                           ,                                                                                                                                                                                
                                                                                                                                 .;                    .;,                                  ,.                                                            ,;:                                               
                                                                                                                                 v;,         ,        .;.                                  .     ,.              .                                         .;v.                                             
                                                                                                                      .                     ,.      .,,                                .;;; .     .                                                         .vV;                                            
                                                                                                                                            ..     .,                                ,.;v;,. .           . .                                                  :v:,                                          
                                                                                                                                    .      ,A  .. .;          :.                                                                                                ;v,                                         
                                                                                                                                   .;.       .   :                                                                                                               ,v;v                                       
                                                                                                                                                 ,v.       .                                                                                                    :yNcA;;,                                    
                                                                                    .                                  ,YmtZGeqZ@9B6OCCU       ;         .                                                                                                    ;wv,  vHHlrv                                  
                                                                                   .A                           vcjbGB8B8B8B8B8BgB8B88@98BOpyV;v;                                          .;.                                                             .;:    .VrHA;.v;                                 
                                                                                   rO                        vOBqHXC88BBCcUl       vq8B888B8B888B8B8BB6Ocv                                                                .                                     ymY   v:  v.:v;                             
                                                                                   p8                      Z8qUTcO8BOv          v4#jAMky;vyvT4UTv.rj@qBB8B888Bej,                                                                                            lgOv ;   v.. ,LVV;                             
                                                                                   jBv                   gBh yG88Ov            .,      v;              ,. .vNpbOB88B;   @ .8M                                                                              bB8y9w :v   v,:vc   ;v;                          
                                                                                   cDB                 X8Z v88eh              v.       ..         ;;lv;v    .:, ;y:m8q..yBvBOv;                                                  .                v     Vq8Bp  8j: kM  rHw .  ;v, l,                        
                                                                                   O.qG               e8: e8Oy       HUv   ..        ,   ;,   .;;:,,:vvr:           48r, 8eBA X....                                               ,              ;    U6qgc    8v   y  ;Hr,;rv:   vv;v.                     
                                                                                   2c 8O             B8  B8v       ;8eV  :,                ,:,:,..;   .;vv,          B8 :Bc.ewv.  . .yv                                           .                 leZv@Y     BM   ;:   .,.Ul;  .V:. .v:.                  
                                                                                    Bl 8B,          B8  BB  . ;.  B8m  ;;                 vVv;          ;;v; ;.     ;8k.G8  ;8q      ;.c: ;..                ;                     .           .   BBv.ey      yB   .y      ,v,;:;v    : ;.                 
                                                                                    .Bt 9Bh        #8  88  . HC ABO  ,:;                 ;v:;:.           ;v; ;;;   ,TkCv     8D   vv .l8   lYv:                                              :  t82 H8;        gk :;h;  ,,  : :. .       ;.,.              
                                                                                      Bg vB8;      8y e8   ; e pB,  vv                  .  .                ,   .,             8H   B;  BG  .  :vM;        .                                ;v  9O  #8X         ,8  ;r;. ;,            , ;.  ..             
                                                                                       e8  v8Bc   ,B  8U  ; jOpBT  v,                  .  .                                    t8   Ze  CB         vA;      .                            yG#jwqBB  e8  l,   AOt .8p  ;    v            ;;,                  
                                                                                        y8Z   08elvC kB   ;.8888  ;r                 ;l.                                        B.  ,8  O8            lv   ;l;                          kc:.    H8B8.  YOGD#A.  e8;  v,   ;v.          .                    
                                                                                          vgg;   yZGbB8     8B8v ; ,;               ,v    :                                     8L   B. 8G   .;   .:.  ;v,  V,;v;                     yZ8p;      b8H jbcZO;    :8v   vv    ;;.                         .    
                                                                                             UD9T   vYkCZ8Zmkc9     .                     ,                                     Bp   8vUZ  U             vyvvv..v9c.                Uw   :Z8Z     98j4tv :;     X9   :v    ;.:                              
                                                                                          AT    ,t9pk    ;Xm, ; .   .           .                                               8c  ,B8v  l8               vVwl;. v@8          :vvOZ        88Y    BO     v      Bh  ;l    v,v;                             
                                                                                         ;,yGc     8O9@k                         vwkCDy:   .                               :   tB   MeZj cBy                 ;X:;2; t8     :lBeqjCZ8B        88:   tB;  . v,     8Z :rl    v lv                             
                                                                                          vB8B8v   g8O:U8O                           vtB8p                                ;;.  8T   8. ;8G                     vAVCbO82M   96l      2B        82    8B.  .;.     8Z  VX.   : ,v                             
                                                                                            p8B8e   88b  06, ;                          .GBe:                        .,. .    BC   kB  :CB       ,r             ,:   r888c,A         VB       :8t   M89vvHbO#p:  8#  ;k:   , .v                             
                                                                                              Bt8B   B8U  8By, .                           w8G                      :;ycplvOZZc   vBv;,  B8       Ge               ;;  y8B8Z,         XB       v8X   X89kGG@m.  v8@  vc,   ,  :                             
                                                                                               8 hB  :G8HV88b                            ::  0Bw                   .;v;, .,GB;  yOU      GB        8           , ;jp2Gm  8gBB@:        88       Z8.   ZB  :  . .88:  rt;   ; ,,    .                        
                                                                                                8 ,B  .:mM8Gr                             :,   B8                 ;Yy:      .v@Bv        C8        8l          vpB;   OBADB YB8Bw       8G       BOU; kB  v.    8q   rL   ,; .;.                            
                                                                                                v8  8Z;   08r  ;     .k:yyyM;                   B8               ,ry,        ;B:          BU       e#           U8v 40vHB8@   c888V      8;      8BhmUqG. ..    B8  .m,   v;,.:. .                          
                                                                                         .       H8v O8y; ,BO        ,8B96BOtA,                  G8             .vy.       ;eZ            OB     l,Bq            b8j, vy8BZc    pB8D,    08     ;@8yv;;ly       0B  ;r    v . ;;;,;                         
                                                                                          vv      ;8q .8BO;BBU v.    :gH;;c8B#pg,                 #8           ::;       UO9HlpmHtmkcUvVy,v8     .88              28B8Zv ;w2Oeky:HB8Bj    e8      O9m9Z9OO#l:   y8Y M;   v. . vv,:                          
                                                                                          4v        GBk: UD28O .v: :yBY     yB@ABO               . ,                  vp8w    ,lmMNv;,   t B     .8             ,...,ygBt  ;D.     v8B8    B8t     80   :cqCOcA.YBc.U.  ;v ::;;                             
                                                                                          Yv         v98v   B8  ,   pe,; :   .gevZB,              :              ;rmDC;OO               Zc 8     yB                    YBtl8y        NB8w   O8B8m;.Cv       ,,   8HV:   v,.;.;                              
                                                                                          ;;           j8T ;88v ;;  Uj ;Lwc;  ,9O H8;                         :::Lv   TG          .    BM qB     O8. ck       ,.      v28@  HjN.       G8G   BcmjHv         .    BBv   ;, ;  .                              
                                                                                          ;             V8U B8; :4  t8lwZ8BBD  ;8h CB                                Cj              kBV  jB     e8   y8A      ;       ;ceB#  VBek      y8B  eHLc9BOw,           8B   .v:v, ;                               
                                                                                          . ;             CH8B,  ;  ;B8   r@6G. qOvc8                              ;6y              ee    8X    NBN Ayv#8BCv    v;;      :vB8    CBj;     8BO    .yYUZBGA       vBm   ;  . ,                                
                                                                                         .  ,           .  cB8,   .  .Bj    C8D AeLv8l                   .        O8:            kB8:    LB;  Lkj8h ,.   LB8r    .Oc     jZ@BBm     mOUy;  @BO         ;CBGv    B8   :                                      
                                                                                     :  v. ,  ;         r; k8Bh   .   :8y   vBt CZ:  eb                         v;            LDZ;  :vvVtBT  L8  t         B8j    ,BV        m0BV    v; vl; NB8              ;6B8r  :  v                                    
                                                                              vv.,;;l;  : .   ;         :v 4B8#    l   ,BB. wC UeU 2 vB                     .          .:Vlb9y.      j #Bl   8  Z,          68c    DB,      .V.vBeC0;        vB8;           v8Bpv,.;  ,  .                                  
                                                   .he8B8B88888eZbZbtc0tUXmrv;;,,;vwV..v.                 ;lZBe;:; v:,;  YBB8VNbUvDB  8                         .vM;vvvlv. 9k       .B     :B; 6A            B8v   Y8thv.     . ;88UHv        :88l        ,B8BB.  v:                                        
                                              .vZq     :vAL2bZqOe8GO#qU4vvYMhjjpOO##Y;v:                   VX8B; :  L :v   lCOcvH#c   Gb                                   8         8X .0OH  BA              B8;v ,BOm8Bc        @Z vOZ;       B84    ,t8BOl8g ,;  .                                       
                                              .  B9 .           ..;AbOBBB9ZDjHL.   vwZg99jl:.              :v#8c                       B;                                 8:          9C@A  9B,      .;.Hmyv, ZBL, ;e:  wB8t       e8   U99,     D8ZcjBB8@  j8v    ;.:;                                     
                                               ;  Bm .                  ;yc#98886c.      vjeeO0U;     ..    vtB8v             vy        #                                8y         L;    j8U      ,;;  BO    ;8t  UB;v   t8BU.     By     YXyv;  q8eY     8e.     :;v.                                     
                                               v  OB                            vbeB8q,      vZB8Dm:        ,cm88w,   ,       ,y:        ;             ;               ,8y             .XeU            y8v    ;8#  Z8y,:    8By;.   8G          2  w8q    pBU         .,.                                   
                                               ;v L8;                               ;c689h      ;kZB8@       ycOBe     ;        ..                     A              t8.            VjX               8B     m8v j8Gv  ;    8B,.v  r8M        U.   ;88  bBb            ;                                   
                                                , ;BH      ,v::v,   ,                   yO8BO;      l98G;     ,O88:    .                               j  LB.       .OH           ty.                 DBj     8# y8Bv    :    89  L  M8V      8#      8Bph               :                                  
                                                 .y8t          vUpZeB8eBGUvv.               U8BB4      NB8p;   vB84;   ,.                              CX  lBB    ,OO;         pk e                  CB8  y;v6G  DBc      ,    XBl v  c8.   Z8,        eB                 :                                 
                                                  4BG               .vLy;vkNwkZqkrylv          k8B8m     ,c8Bk  LB8Nl  ;;,..                          .vG    B8r Yv            ;,kc                 pB8;  8O    C8l             9B ;   qG re4           8B                ,                                 
                                                   B8  ;          ,            ;vrAvT2HkTY:       m88D     .vZej m88v  ;                              v Gv    pBp              bm8                 OB8v    rLvAOX           UDZV O# ,  :B84              8B                                                 
                                              y,   v8t               ,                ;;AcZbjUHvv   :O8C       XeG8BB                                .v 8;    tH8l            cZ8;               H8B8By                 vcOjMY   #8pc.  8DeU              8B                                                
                                              BH    B8       :      v                      ;;vyNcCpA   VB8r      yt8B8H:                            , ;:B.     9V8           HB8A              ;e88e  UD9OAvvv,:;VMbqGB8B6t2tGB8qtB0    B  8BCy;           8e .                                             
                                              8m     B8H;v   ,V,   :,                           vU0ttA:  ;G8k       8BD                                v8      gw9          HB8M             .GB8Bm          .AwtZOtBBA#8kcmmv..  8    V8   HZUpOj;         8Z :                                            
                                            @ eB   p   Oe8B  .hr                                   ;VkwH4v ;G8Z     y8U  ;                           v ZG      eCO         q88:            .B8BBv                   #O Bv      .v9y    8;     vV,  ,r0v     U82 ;                                           
                                            9 GG  YB H; .48B;.  v                           :        .;;lmqr  rBO    B8l AV .                          Bc;.     BB.       88O           tG8B84                   . r8 Zq        Hq   ;8y        ,v;       .; #84 ,                                          
                                            Oy08  08.gB   D8D   ,r      ;.;;vyXmNkM4N00OOg6OZZ2UATVyvv;.  ;qDH  c8H  N88..;,.                    ,  , O8 l; .   H8k    vO8B;       ;tBB88w                     .   8mYB          vjXrr              :.:       q8;                                           
                                            ZTc8  DG t8 vvvB8 .  vr  .;v;;;:.                ;v:vvYNhTUmpmL.vAUyYT9Bv CBb , .                      ; k8;.Vb     cj0vvb888B6@eB8B88888w;.                          89Ue                                 k    L,XB8w;                                         
                                         tV ApT8; t8 :B r8 wB#   .vV                                    ;v;vvAv4k4H8Bqr8Bm   ;                    v ;8t  Y0C    ec#Dcpy     vVvv.     #                          8D09                                 X6  X    .88                                          
                                         wG .6NBb lBk GO 6p OBv l;.                                  :,v;:;. . ;UTHjwUCO8BH   :                 :;  8O  ,  v   ;8lC                  Ue                         8O9H                                 OO  9v     :BG :                                       
                                         vBm #Uee  8O m8 vB :8Bvvt.  :.                    ..:;CMLv;vvvv:yHjZ8@ttcwklh;Ne8Bp                   .  ,86          GB 9  :               8;                       c8BD.                                qBV  OD       yBb v                                      
                                      . TlGB hO08v p8 yBt 8m r8BTy;    v         ,      vlrvv:;U4lvvArAyq#Uvy;vl. wjZrr08B8BO     .          .. :88O          .Bt 8 .:              BB                      ;e8BX         ;    ,yvv             MO80   Bm         @Bv ; .                                   
                                      ; hGj8; GjBO ;Bv 2BMc8; ;8BY  v;  .      ,VV;htwl;,,.   ;;,               ;ky  vcUr89ZB8@Y             :@8B9  ; ,.      O8 U9.v              yB;        .            eB86;          ;  V9A::rG       ;rbOeH,   r8:           e8  ;                                    
                                       ; BOB9 Hj2B, OD ,emhepT  8Bc  ,    ,   ;vyAkyY;                       .;:pOjwvwvrO4;,DB8B8jUV;.,vAkOB8Bgv     ;Nv    wB8pYBt;.             ,BH       ,.          ve88q ;v;;Actm4VvwthbBl    B8mmHGhhy..H6t   mX             ;8B                                      
                                       ; ABZ8U DXBp .8. jclZ88c  eBp        .      ::                     .;vV4cv   k;UB.  :qt cB8B8BGe8Bejv           ,  .; 8BCjv;,             .Bq      ..          C8B8X     ,yl;;vy2#OOBBk    U88v      O8Dh.DH;                v8A                                     
                                      Uwv,m@gB.VphB  bC ;qHmOBbV  VB8p   V;   .       ;,                rcALAVv    k;t8    .gB. UY .; y,       ;.,        vC8B    ,              B0              .r#8BBy           .VVv,, ;O8,    cBH     tB9    89:wv               A8                                     
                                       eOUy@ZBgvTkpc vZh ccmH@C H4 ZBpOOG8B8Gv         ,.    ..;     rwrv,       ;Dp#v     ,b8#  C;    ; .      ly        #8BB                 vB:            vpe8g8.                  ;Vvh8e     l8@.v: kBh    .8;   ymHT; :         GG                                    
                                       Y86cH0D8C U4b;v0m;vchbje.;YXq8:vgUB8ZGBD  :.       .:      AOC;          HBOh        jB8v ,8Y    ;.       r;     ;88T Bv               DB   .yrNmmUmDGOjv   C                      VB2      8p    Bj     0B..     ;yj9OZGZD886hc8v :                                 
                                        UB9CtZCZV cCj 42yVUX2cqmy.k@BBq8hCBg YB8A  .            vep        ,   D8m.        kBZBm  T8     l;            DB8  A0      .       U8O  ;kHryvVV,        #l                     r88r      9#   y8      Gb;j;         Ne     . OB                                   
                                         jO9O0#H2v;beX jmvMXUcrg8pbvVB9eBZgBq OB8OvV0pv       .v;         .    v           p8UpBr  Ay     T          U88D   8       :     H88;                   .8      ;H,            bB8;       BZ   VB      8H g4        y8.       ,8; .                                
                                         ,jH#eO9ptVrC8h,jvy0AmUZOpBAr8eZG8GGBO@eB8Oj68B8h                         ;; .   ,vgBjY6BqMrT,          LmU8B8O:   e;          ;O8Bv                     8U    :8B.,p         v8B8X       Z8   :Be      Ot HC.      08v       ; OB                                  
                                         ,e#mjee9pqyr28HykVtC,CeDTGVO20BZ08B8BBBC,    NB86kv,                 vO888B8BBe8B8B8GH2BmO88Or;       H8B8BGU, ,Aev       ;@qOB8A                      8e   .88    O;      ;D8B8gv   :  v8v  ZBU       Gt  @l.    Be         v UB;                                 
                                          XBBkvcBB9Zryj8#mHmqyN86g99Bk m0pZ888888r      .B88qcCCq0U.         UGy,MC8B888B86eG6hZelr89AY#B9DB88OB8O;  e;      :r9p9BeOm qZ                      8B  ,88t     2U   ,29q9Bk@j ,     8e  pB.        9,   9m  tBv             8l                                 
                                          .l0Bp:vqeO9Nyp8G@CZ0b2G96B8v ,. ;; vm2V.       tGB8B8B8B8ebtUANXcZOA  ;ce8e6gGee696gbO8;;vYh,    :   b@   48              .  ew                    ,8O  e8b       8clCO0L;tCNvGT      .Bv  BC         8    p8: ;H              99                                 
                                           ;hUG8H;y2DeZUp86GOeOmpeB8O   Y;               9eeGBB8BhM0e888B82Vtv   kBB9ZqOGee9Dee68H   .wh.   .NO;   CB                 e8                    m8@  8e       .86M:  yBBCce6,        8; U8         8H    .B8H  pc            pB                                 
                                            ;20Z8e0jcrHqmMZ6gD9O6OeBH    :               88gBB8B           v8m  .B0ZOmOGOteZO9e68ev         .     eB                 e8;                   GB;  8@       @8U  MDe2v wBc          8v CB        ZB.    V8vGB; 80           :8,                                
                                              ZB#9BgCmUrUtCe66OeeBB6                     ZB8B8D         :c68v   g; 8AcBBty6BO99gB8Y             v8e                 B8v                  e88t;:OB       X8qj9OV    ;BX          melZc,        B      UB  qB# #A           Bv                                
                                               ;ZOB8gtZ#COeeBB8B88j                      :8B9v    vq; vpt;#8.  l  bbvZ8Bk;eB60bGeBA           .O8k                :B8v                ,B8D.    8D        jt     GB4Yj          t8BeV         H6      Ag,   98@BGHv        Gc                                
                                                v8cU08B9e8B8B8BGH                 ,.      G     ,68tt8Z  vO8l    ,9yvDO8T O8GCy9G8c         ,G8t                 0BO               ,m8Br       8m         e  ;BG;             :8Bp           8y       OmX;   O88BZvh:,,   ZC                                
                                                j8OL;vDBclAy;.                ,   L98OcCGBC    v8CpDrO8e8B8B     q  VOjBY Bg#c.ZeBBv,    ykDDv                 v8B;            .#8B@8y         8v        y8 jBv               VBv   .       ,8        8 er    ,O8BH  lAvy;#9.                               
                                                H8ee9tZ8q.       :HmM.               .Vh.      8q@t    Lv tC    v   jch8  B@UD:0ZGBC;CbjC8Z                  rB8j  ::VAvAC2DO9O2v   D          8         GB8B                 gG     .     Z8.       8r #b ;    h8BB;     kB;                               
                                                .BB96BO984;cjMjB8B888B8BC                     gBGk  ;    Uq        ;g re  8m;O;pH28@  v   Hp,             .O8e,     .Mk;           8;          B         j88            ZG   C8          ,8C        8O  0N ;v     c88@,   vBy                               
                                                 tBgpjqDBGm8B8B8B8eBe8B8B8v                  H8el      ;bL         cH bH ;Bv O kw@Bgv. ;   ;yrv;NAyv;:vH98Bt                     vB;           8,       .BBr            :  ,B8;          8:        8O   :O  U8v     jBBr  v8c                               
                                                  qBgq9GqOCtOO9B8B8B8B8B8B#v,,               t8w   ,YCGU           U  H  .e  Z ;kU8Bh,  .       ,;TkHmmq8                       ZB             Bv       ;y                 ;r           8;        8L     e    eO;    ,bB8v GZ                               
                                                   98B#O@ZZM,  ,;VAmDObtY.  ,vAUv,.          @ZrAm4r;             ,;     :C  k .l O8j , .  :  .        9w                     ;89              8Z                               . .    8v        8l     08r@j, hGOv    Y489ec                               
                                                    eB6bGG9OHV :.             ,yZgl          .                    .      rk    :  UB6v .   ,          ZB                    :B8.              yBG                               .O8   8e       ,8.     c8   ,meG9B8Z      0BG                               
                                                     mBB0qDBBc       .   .:  ; .;tgy                                     Y:   .    88H  ;  v;   ,    8B                   ,69             vGOy vv                                v  A8G       ,8      Z8        lZB8BD      B8v                             
                                                       D8e6GBBc   v0Z#2COC0L,     ;ek                                    ;         O8C; ;y vA      m8e                 .CB8g      .:lwOB8ek.   2                                    y        y8.     y8  9v        M9B8O     28B,   .                       
                                                        v69888Be   rtDmUt86pD6p;   .BV                                       ,     mBBy  :; ;;   G88v                hB8Z   .LTUkptqUvOv       8.                                       tv  .;       8    e           H88O     ZB6                          
                                                          .;XvVbBcqp,    .Xl#89     vB                                      .       8Bjv. ,;;cCG8qqXV           ;XgBBc;:              O        Bv                                      vH           yZ    ;c            j88m    .B8U   ,                    
                                                                k8BOy      Cq,       Gp             .                      ,        q8@cb0ZD889L    ;kbpUcO##OqVOD                  ,8v        8y                                     .            yB      cX             C88b    k88; ;,                   
                                                               A8eeO8e996GZv          8v            lH80                            YB6    U8H ;                B                  U8v         By                                           ;     8B        C,              0B8;    GBt   ;                 
                                                              @8BeOeBj                A8               eB                           ,8By lBB: ;.;             ;8p                 98,          8U.                                         #Y  Yq8@          c         ;     :@8e,   HB8l  ,                
                                                             B8ce6Oe8B;                Bj               8                           vC8OCeq   v.;;           D8;                .88            BG                                         ,   8Bg,     N      m;        .    : .8Be   .g8m                  
                                                           #BU cBGOee80                ;B;              y                          v, B82r   MY .U;        ;B8                 6B#         kpjpDB                                            8w      kB8 ;.    YN                yB8T   C8q                 
                                                         NBU  eeOOOOBBgVv.              ZB.        yeY                             N  0BBr  ,;V  ;N:     ;88;               vG8j        2B8OXU8  B;                                        ;86l. vHt9v B   U.   .G          ;      O8e.  C8G                
                                                       ;m,  j8B.MG#GeeeeX;               GB,      Cy                              v;  ,BBc  ;;.   vv   Ze9,              v68B8;      cB8Be    B   H;                                      C8jv#jMjBZ;  8    ;m    e;        ;.      ;8Bm  X8B               
                                                      .   mB8 ht09DZGGeBew;               mBr    AG                 ;:           v;vv;UC8BN:.L..  c#8B8Bv             me8Bev  v;w09B8B8r      8w                                         B8v      8.   BC     k;   OT                 9Ber ;8B              
                                                        vBBr  ce9GDOG6G66Bp;                98kvAv            :mB8B8BgOG    .Z: :: :;B0vZ8grhqO9B8GGjA  vht2jCpDGBB8eeBl        O8Bk           B                                       mB8      ,8b     B      .T   #Z            :    CB8c l8B             
                                                       C8j  lB6CDOOG696GeB8q.                            ;0G8B8BOZ9ZOO9C   ;8A    .M8B, ;B8.   kBX   .:              yU      U8BG                                                    v8BG.     Z8B       w       k.  vBH          ;.    l88O.;8B            
                                                     l6D   BBmtD9eOg9O6e6e6BGM                         DByr    ;          C8.    rg8q.   Z8g  c8Z     v             p8     @B8w                                                    ;B8Br     jB8j          ;      ;k   B6          ,:    ;9GGvA8Z           
                                                    OZ.  C8mVUZD@CeOGDeZOOB9BBO                      vAH9                       jB9Y     v8BOODrv  .   :           B8    m8B;                                                    .eBB;    Mg8B9             L       Ov  CB          .;    .ApOUt8v          
                                                  HO;  ;BD ,bGZOUcUZOCCbb#98BcB8X                   Z8                       :m8BH        c8Bw   ,  ;  :         qBp   ZBO                                                     lD89    w@8B82                U       ;Dw vBr         ;.     ,.tj6B ;        
                                                AD;   OBA ;tbZOjO;;bpCZ#hH88y  08BA               UOrt;                     Ce8bv;;vv      eBDU         v      m8B, .c0X                                                    yG8BX   r2eB88m                   V;       cg:;Zk                  4Be  ;       
                                              .Ov   ;eC  ljctZjOM:mX@eg89eB8     ZB8b            OBG:.;                  gDeOY,:,rvve8OH   veBOLMA.Y:.   , :V@BeMMZB88@#GM                                             ;vje8#v   rD8B8BD;;XTy.      lYVe9       A        9OUZj           ;      ey          
                                             Y4    cB:  TpHtCpDX;pHA8BBGjr;:       ;688#;       vB8H                    vBM   ;     rkOB89; j96H  ;4Ge#O8eDCG0OOBBepr;,                                             vZ8B8Omv;V098gqyv      vmOB88889@,.Bv   .    h        v696@          .      8v          
                                            vv    cO   kp;H24t  H8e@XN.               v8B8B0.    .                     mBt  :.          j8BBjOD9k     :9, ,Ue8gk                                                 Z88cwXv                               8;    ;    v;        28BG        .       GA          
                                                :Dw   mZ;XZlUvvB82M;                      HB88GZU                     ZBO...              cB86GZO     j;ve8Bt.                                                 .G8Z.                                tr eH          ,y         OB8       Y       9C          
                                               v9y   TC,VZ ;g8BZ:;,                          ym#B88eb.             ,G86t  .                 m8BBZA   Ht8BC                                                     0Bl                                 #B  .B       v   ;v         vB8;     :       Z6          
                                              v9v   VC, Z  Z8#A .                                .hCDBB2v       :0Be80Y  ,                   :8BBb  ZB8h                                  9                   pB;                                08G    O:       ;    y          t8q    .       e@          
                                             ,9V   vU. v; 68U, .                                     vH6B8BO#e9BgjM; ,:                        vegB8By                                    BY                 q8.                              A8Be.      m         L   m          .88          ;BZ          
                                             c;    U,  ;vB8N. v                                   ;cO8888B8B888BBO9hrUv                         DB8m                                     j8.                8B                             v6B8k          c         w   N           OB.       jBO;          
                                            .;    h: . vB8.  v                                  ll;VO#v    ;vyrHhmC888BB0v                     9BC                                       89               ,8B                           Y6B8p              p         :r  U,          .Br      BC            
                                            ;    ;X.;  j8   y                                                         h98B8BV                H8B.                                       DBp              ;8B         ...       :vYrUD68B8h          .       C          0  X            Bk     8,            
                                                 r:vw.;8t  r                                                              v6B8@X;;          O8c                                        .B8;             w8e               @8gAHCNy;v,   lTY,       .  v8O    c;         vv ;v           Dv   lB             
                                                   yVlOBL ;;                                                                 .O8Bv;,       B8;                                         B8w             q80               O89              :k@OHyVl;   80 w,   rl          k  ;               ej             
                                                   ;:kepllT                                                                     T8C:v     88                                          B8t             e8k            .D8Bt                    ;Z@CCjcgB.  :k   vL          ;: .              8.             
                                                    :cBhyjX                                                                       #e  r  e8                                         U88y             B8v         Lq99t.                            v.Z8     U   ;w           V              w8              
                                                    :jDvMt.                                                                        .B  TO8.                                        8BO             tB8,    ;;;V9:                                Njv GD      vv  ,m           r:            eZ              
                                                    ;hZvXc                                                                           m  8U                                       C8q           :  8B2          v                              lO9v   9e        X   h           ;:           8M              
                                                    ; bHv@                                                                            :pB.                                     ;Bg             XC8B.            ,V:                         m8Ol   , yB         b   H            ;         cB               
                                                    V  C;jv                                                                            Bt                                     OB;             M88j                 ,;                    ;6BB;        jZ         U   N            v       XBO               
                                                    y; Zt4Hv                                                                           8                                    :8B              mB@                      lGkv,;;,     .;AD8BBU            hv         .v  r                  ;B;                
                                                     :XB8b .H,                                                                        .8                                  ,B8,              UBA                      w88Y ;rh@GOOOOjmU                  v;          y  t                 OC                 
                                                     AlbBBt  .v                                                                        8,                                gG                lBc                    088#                Y                  k.          T  r                8v                 
                                                     yM:eg8Dr  ,v:                               .                                     y,                             vcV                 yBO                 .yp0l                    OZ           v.    e           r  w              #B                  
                                                      c mB6600Y   v                                 .                                                              .;                    0BG       8G  ..vvv;v.                         ;BBGCy    :qOty   Yb           ;  M            q8                   
                                                      vM ce9Dv:v                                                                                                                        6B6      cBv       ,                              vU#B8BGB8Bv r0   g               ,         ,BBM                   
                                                        y j6gOv                                                                                                                       ;8Bj   ;kCt.          X;                                   bBv   l9   e              ;        re,                     
                                                         v4GBe9y                                                                                                                     m8B9vvcCU.              .h                               .. m8.    vg  lO             Hv       8                       
                                                          ,peG9Ov    ,                                                                                                              989                        ;;                           ,y   lB      V6  OY             9      v8                       
                                                            jB86ek   086,                                                                                                         vB8h                           ,v;                      4p,     8;      U6  e              v     eh                       
                                                             .q8B8#.   @B8BO;         Y0OOh:                                                                                     8Bg                                ,Vh. ;             pGp.       He       GM vZ                  Te                        
                                                               ;eB8BH,   vZB8B8O02eB8Bgc;                                                                  .                   D8Bv                                  ;VvjqCkXyv;;;vqB86:           X.       6  e.                .B:                        
                                                                 ;e88b;,     ;;888A                                                                                          8B8;   X                              Ul          ;v8B:                U        g  9                Bj                         
                                                                   ,qB86cTv  .; gBp                                                                                       vZ8e.      v:                         Mec               ;Y                w;       Uq 0l             ,ec                          
                                                                      C8B8BGkGg  mBGl                                                                  v                j8B8H          ;v,                   vZ8D                   VAv        ODA   9 :      9. e             Gl                           
                                                                        C#HO8B8Bty0B8jV,;                                         vjv              .H684      ,      ,eB8C. ZO ;;:,;rwj#8GX;;:  .        LDe9Cv                       :k@pZ0908B ;H  ZT        t q        mZvHX                             
                                                                         :y9m .bB8B8B89mhv                                          VBB82mLv;yUZqGB8B@       ;   ;U8B8O.  y  8t  :,,;        :N69CO6Up@8Z;                                ;0  Bv  O, ;9.        #l       bt  .                              
                                                                           B8;  .y   ,be8B8BDL. ,..      ;v;                           .X9BB8B8BOcU;        ;rq@gB6y       t  G8                                                        :mT  ,8   :#. el        v#      6e                                  
                                                                            g8m          vm@g8B8B8B88888e,                                                ;8BA#8c           y; :Bj                        ;V.                         mer    tg     e A#         ;     90                                   
                                                                             O89                ,,;:   vmq6OOpkyv;                                      VB8B8U @8el           2  b8;                         vM;                 .UZ8ec      V8     DN e             YBB:                                   
                                                                              m8Bt                         :yjZ8B8B8B89U,                     :       NDB8e e8  V9B8By         Y   cBC                          ;CD##HYhYvvyU#OBeBq;          B   , ;9 #.          .k8H c8GA                                
                                                                                eB8#:                               .vNZ888Bec;            .VOeB8B8B8B888B  ee      4B8BOw;     qeAD0;et                      ,Av,  ;vqO8Z...                 DX ,;  D Mr       vj6B80   yvLZM:                             
                                                                                 ;B86Av                                     ,4qgB8B8B888B8B8y   :YrA;ce6BH  B@    ;    wB8C8B8B8@G6ec   ykv,                4eZ          OU                .   c v:  rVrq    :C8B v8B   v.     c9v                          
                                                        qcyvvvy,                   yeB80c;                                                           ,8GG   BZVbA       tB              :;vc6OeCcMTvkwYw0Ge9M             :g9,           V8B8  j     .rvU  m88bO8 cBA  :Y        .gA                        
                                                       NO89geeB8B89Dv                 C8B89r                                                         tB8h9B U            GB,              .cr0OB8BZOZkyv.                    Z8Br.       8v 8  D         vB8Dvv8U B8   X           vBj                      
                                                       mv        .wZeGU.                Y@eBeO2y.                                                    68D  8B2             CB0              ;    vC                             vgB8B8OBjDe  Ol c       U8BOy  eB h8j  rv             q8                     
                                                       O             ;9ge8BOMLv.           mB628g                                                   Y8B    ZBq             vB8l           .;      G4                               ,AH;:8Y  vq v    .OBBV  :.jBM 8B  ;y               ;8j                   
                                                      OC                  :jBB8B8e@jcwVjqUkq8C                                                  .   eBp      G8Bv            O8Bl          .       #@                                   e   hv   qgB8q.   ,;MBB kBH v;                  8@                  
                                                     VB;                     .NUNmcbq9OZC8BUC8m                                                v.  8B8eeGy    v8B8G;           ZB8H                  4U:                        Yk     vm   h  qB6v       v.68  BB  ;.                  :8c                 
                                                    YbB                           vqHVXMNM.  88y                                            ::    8B8U rGG8BOL    0B86j:         28B80;                ;Uy                   ;mOL      y.   ,,8Bv        X;b8T B8: v.                    m8w                
                                                   lO8v                          .j8B        ;B8X                                         :v;   98e, 06.           ,BBNCB8B8B8egjV; j8B8Bg0jtY,  ; .::,    :tjv:      ;vceeGUv,        ;    j8q         Alk8O 08U lv                      DB                
                                                  ZB8                       .       8Z        rBew                                       vv   cB8.    OBm           lBm                ;ZB8jpg888B8eDY         B8,lUp#2A;              AYNZe9.         ;,rBB4p8t ;v                       q6:               
                                  v,.y,          DBV  .                       ;     U8         ,BOt                                    :.   t8B0       qBZ            B8;                M8B.                   Be                 hD9B6cBBl          ; ,OB9e8C vV                        MGc               
                    VB888eBge9eBeeeg8B888B8B88gqVp:                                 CO           tUX     :.                          ,r   ;B89   vmkc   yB8U           r8B;                q8BT                  B8b           ;Oe60Al, vl N         ; .@e6e8h ;:                         ;DO               
                     Z.                    vYvcDGeB8B8Ok.                        ,lZ8Bql     ,   vDgB888B8B8BG4                     V:   GB8V  ;Z8BO      b8BD           b88y               ,8B8p                 ,8BB:      OBp       ,A   TeV    .:  jBeB8M v                            ZG               
                     l.                             ,vjD8B88GU:               .cql.v   ;     rZ88B8Ojm;.   .rjOBe   ,Lbw           v   H8BH  ,DB8    O.      O8BC.         y88GV      ,        C8B8m.                keZk  D8Z       Y;       #B8eGBj;t8BBgr,                             .qB               
                      O                                    ;U9B8Bgr          8BH            :GGGy              HB8B8B8B8B8Bj     ;,   e8C   m8ON; UyvG98B8Zy,:UBB8BOV,       .q8B86k.vv           m8B8O#jyv;            t6BO,      .;              A.wB8BOY.  :                           .O9               
                      e   T                                     ;p8886U       CBG            ;              tB8B9tr       ;cBBU;2   HB8.  UB8# vHvO e9;  y0l;:     #8O6B8B8BeOOj.,bq8Gee99e#CjHV:    68rlpOOgOOZUjmv  U9Cv      ;A8G;vAv;           UB8gp.   O8                           YGq               
                     L9   M                                         ;2e8Bev   ;qBB                      .2888G:            lq8eO:  BBC  .eB8Z v9 CqUXOBN            Bq              #C .yYrUtC2kv     jg           .A8U    .;;HO;HO .          .;  pB8Zv     B6                           b8                
                     GT   m              ;.                              :#ql;  qBG            .;;.    ,X#X,    .        O8D .v  v8Br  t8XtO ,e ZO,b; DBGv          ;B8.            vBe                 8e:       qeD  .HNv .     j.          ;A;A8B8m       B8                           Gb                
                    t8   h:                  .,;,,                          HV:bB8B8V             ,l;;v;:        .,    UBe  :M  v8G  .B8 :O  8 .@ c8. .r88t           D8e             C8G                lBGv; .OB;       lM       #.       ,. pB8ek;        8B                          U6:                
                   48;   C                           ;vv                   ,  hB88A28Bv                 :vYvv        .8By  yh  c8Z  CB8 .B: BY 6 vOB    ;O8B           ,8B8;    .       eB9                NB8g#         p8,        vCV   U9eG8B8q           v8H                        ;D#                 
                 ve8l   9                                vtkLL            ,   Bec8y ;888ZM;                         ;8D   Yh  c8C  80B, B; ej 2v;plB ,    veB8.          ,98B#v           68B;            UZD:         :#c           VZ868B8B#;     ,         c8N                      .C9                  
              ,C8BH   hj..tmv.. ;;                           ,vv,        V   G8U B8    y6B8B8gB8BB8BeZ4           ,e8V    ;  H8c M8kCY OO CH:4p 6;w8   v     @8Bb.          vC86Cr.         ve8Ov       OBb      ,vkjAZBj:vcGeBeGjv,.vtB8Bl         v    Z     H8h                     CO                   
           He8gX     jCce8B8B6OGDGe6bCZOe8B8BBegODOZcXV,           .    v   y8@U MB;                .UB8By       ,B8: : ,;  Y8m k8NrH XG Zm.,C O; X8  ,        m88BOc;    :.  MBB8B86GDqmrrv.vYyO8BD: A8l   ,9BGjh.    lBOqj;       :8Be;               p8      l8e                  ,4l6                   
         UBG;      keB88M    ,;ZC  BG           ,,;yrrcCZZODOt4y;     ,j    8V h. 6B                    8Bc     eBT    vp2,X8D;j8j;#  8 ;t ;cvNv ;jBCGA ;l .rbZcv98t2B8B8B8B89tl   BD ;y9e,:         Z8            .    :#         k8Bv                 CB        8Bv                .  8                   
    .  v9eM      Ubg88v    ,,   j2  Bp                          ,vLkHp6Ojh CC   Y  8k                    98  :B80      ;, .8Dc9D9h9M Oy C  ;jT#  ;jBjZO8B8BGN.   .8O               ;BG   ;v          8:           B2     @O      ;O8B                   v8         .                  ;8p                   
   v  rgy     ;mVv0BZ     H     ,B  Hc                                 ,LZOg9OZbGT  6                     thD8Z       ,,  DOrOlwVcM 2Gv9, .cA#  :vbBX   v          e8k               986   C       M88           BO    ;. .6j   ,e8e                     B2                          c8m                    
  v  29     .ZA  #8A    .v      ;8, CB                                        ,kqeeO88;                               v  me@e; m:2  # TM:;NjVT  ,v28B;        :     ;8Bw               088: rq,   B8M           6;    ,.    O2H;#8#                      vB                         #8h                     
  : mO     2p   cBy    :;       qB  8@   ,                                          ,H8B8Oy                             .BZBU jvZ  vjv0  .jYt   ;:jOkB8v              mB8Z               A8B@c2MOBm          ;Hv          rr  HO8@                        cm      .               r88N                      
 ; Ae, v M9    .BD    vy        89 bDCN;vgj;:;                                           cB8B80v            ;MHb09OOO8Oc9ZGU ,pC;  #m0    cct .;X 48 vO8BX              .eBBN               .  .B       vv;Y:v      vjpOgDp   ,8e                                                Cvy9                       
l :Bt   Og     q8;    U        G8  G v    v         .                                         mg8eCv ;298B8B8B8D0cv;ck v89G  m2j  VCC    vjq ;h   kBUv  #B8Bpv.            t8Bec              yBA    VXl. .  ;8B89eB8Gj   .   0Bt                     C                            ve                       
v be   ;b     :8D  , vv       e8U 9rV     v                                                   ;jB8B8eZXl           y6c Z98.  ZZ;  tmV    Uwv:A    X8m      c98BGCjvv;;;wj#0ZUGAtB8BeU;       BBv .G6hrv    8c   D9       ,  .vB8,                     ec                           ZG                       
;vec   9      UBy    y      ;B8: @qv,    ,v                                             YCBBeO2v                TD86y yB8A  bO,   OM    vLrl.   :bBBh       ;k8Bkmee8BeZCV   bG   ;jCG8e9tDB8q   NV.      je     V8j     ;  ;28p                      Oq                           6O                       
:rBy  yr      j8    .N     k88. h6;      ;                                        lCG8BGtv                   q8BZ2    g82  vGA   .Z.    .Hl    CBmh8qOB8B8BBj;v8c             e8c        ,B6             BB       :OBO.  :  ;OBr                      #B                           8q                       
;N8;  r       OB    A     B8C  Z8;      :v                                   ,kOBeGU,                     ZB8y.:     k8e   4q    vT     ;r   k88  ,B9  v.       8B             ;8Bq,    pB;            y8r           v8B@C .VO8:                      Ge                           Bp                       
,rB;  .       8p    v   v88.  j9                                         ;m6e9X.                       ;B8@   :     v8Bl   Oy    h,    .. UOB8v    8BC;       :  D8B             ;e8C  8B          vVt9v            :CU ;O.v.eB                       BO                           8t                       
,,9C         O8    L   O8k   DV                                      ;Ze6jV                          j8BV   v       6By   .9     w    ..0B9       DBB088v     ;    O8B;             MB8C        yjc              ,ZB8m   .  ,G9                      v8p                          ,B2                       
 . @;      ;O8h      hB6    O.                                   ,49#M.             ;.             9Bg     .       ,BC    vy     . cqGB8BM      08OZB: DB8C          jB8k           ;8k      .Tv             .b8B8Ov     :  Z8#                      e8:                          q8j                       
  : v,  :rBB8jM;.yw eB;   ,e                                  y#6O;            .;:               B8j     l         68     pV    ;e8BMvLr.     .B8c U8H  vb8BO;.         0B8C;      v8j      hZr;TvvkGUeBOB8B86b;           v8eG                     q8V                          h8B                        
  ,; hB8B8BeOeB868B80    tH                               ;qe6t,           ,rV                 8BC     ,v          bD   v8BO:UBBe@;          BB8;  yBO     vq8BOb.         wjDDG6  8p      ;v      YBbwcc.  .qA         .  0e6B                    yB;                          ,eB.                        
                :06y    gl                             ;Zgek.           vLv                  e80      U              ;teBM   09  .        :B8Bv    .B8ZpZjh, ;j8888BBB8eeOqjpZBBy 8p,             .BM C0ty: . VOOV:.YH@B8q;6et6j                  vj                            9Bm                         
               UBC    U8; :vhNvvv;;;;              .V68ec;          ;UUv                   G89       0                 .    vB ...      U8B8:       @8g9jgB8GOZH  88,    ,;vylv. OO.             vBC   jOODBDbj9BeB8B8B8GCCeOkC8l                AA                           .ZB#                          
              BB;    cy                         ;kgBgm           vUM.                    j8B        H                       8j.       9B8j      .   kBGGk.         O8C           kq             CBl .     Ttee8O0qD9Zc;  ;OgO0B88                y                           ,gB#                           
            v8Z     c                         ;@6Oy           ;tY                      yB8,       .,                 .     O0      #B8BV      ..:   y8ObB8r         .BB8;        vB           ;OG        .UZBmV9         4OGOeB rU               .                        0  OBZ                            
           c8k    t4                        vUbv            tL                        B8X        .                   .    .9    CB8BH         v.    ;O8;,98Bq          k8BO;     BG        ;9gO;  ,Mt0eB8B8t;   lp.      He9Dej                                       .  6T Z8G                             
          H8A    y                        ;HL            A9r                        289         .                         yH:k88BM           .;    , ABC  :6B8BZ,         p8B8glC8       CB#.rlmbeBpyY,           vOpv;. tg6D8Y                                         ND ZBe                              
         Y8v   ,.                       :v:         ,v MO,                        y88                                    v8B8et              :    .  tBBc  ;yc8B888g6OODOZBBeOv:8r     c8m .Mcy   ##                2B8DOOg99er                                         eljB6                               
        ;8X   v.                     ,wOB8OOOby      vZ9.                       ;G8y         ;Ul                     :Vg88BM                       :8C28c  ;     #BOlcmtrv     D8     6Z.vbCAO,  ; :ZGk;;      ;ytOj:    yeGO6;                                        CBOB9                                
        8O   4                    rO8B8BDmHj96ObGZ;      ;v                   :b8C        VAv     ,v             ;OO8Bek;,                 .     ;jB8 TBD   .  L# vBv         :8w   hBGYA    B  v.    9B8B8B8BBZN        ,eOeG;                                       yB8Bq                                 
       #8;  l,                   8BgCb         VB86BD.                       jeq       ,#v           y      XDqC9Be8.                     ;    ;kBB8  .8By  O96C:  .B8r       e8,  9BpV     @8 .v   weBB:0C               eGOe;                                      .B8Bm                                  
      ;BX   Y                   8B; v       vB8G;    vtv     ,             ;GC       XOY  C88B8el     qv     v9r    ;;                   ;    mO888    28BZHqV       l8B9    V8c   2        8  m##eB8Ov   .qOtv        :M09OOeA                                 v    e8G:                                   
      @e   A                   88  j,    vB8G,         Yt.    ,           ZD       hBy  b8B0v, Lg8e    8;       Av    p                ;,  vMCZ880     VB8G             D8BN.8B            c8ce898v          ,UqOOO8B8eO; #gGeU                         .       v   #8G                                     
  ,  :8A   v                  88  v    e88;               y             qB;       GB  ;B8A        U8Z   8         C    Ut             yy:cqmUp8B:      ;O6BBqh.            :G8v          l88l;Mv  N              ,j8B     cgG6c                                    C8B  :                                   
     c8;   v            ,;   2Bv    .8BD                   8    ,     yOY       .8m  HB8            ye  .e         2    br          veObU. UB8q        ZBB@A98BZ,           ee         :GBA   ;    ZBOU,   ;AmteBGU       ;B9eG                  .               ;t8By ;                                    
     28    X        vlv;     BB   YB8j                    888       vqy        9e.  bB8               8  Ah         e    B       ;jGq,   y888A        8B2C8r  j88BGqH.     U8v        e#,    .v     :rOe8B8GZB8H           6998v                 ..         ;    ;gBc ;,                                    
     vB    @    vCmv        28  v8B0                    j8Bv 8    ,9H        rB;   GBk                 B  :v        9,   hB   .;l#h    wg8B@        m884 vBe    ,BBkpeB8B#j8B       MZ   ; l,              .l;:b,          OGGBm                 .          .    bBe ,:                                     
      8O   .C :;           v8b 68p                     98B   jq TeZ:       .Oy   Z88                   mg   r       ve   V8   :;v    U8B80;       Ve8B    DBc;HDDkeC      ;Bj      9.  t#Bc.9BX        ,XZeh    v9eCUVllqGpc6g8c                                NB8v:;                                      
      28    mH             @B8BO                   B. B8C     6Zek        qr   #B8y                   .  8X  AV      8B  jB .;.    DB8BZv      ,rGB8H     ,8BBMX;  eB:    C8     ,e  2l  8y  qG8B888B8B8DV         :rkO88   6BBc                               ;B82 ;                                       
       6Z    Vk           gB8D                    e# G8p     t82       ,N:   j8Bc                     ; BBBC   NOv C880  B@     Ag8BDA,      ;,;GBB        C8e.  :. v8B   8O    v8wlv   CB       .88j                 k;    tB8k                               q8el,                                        
        Bt     vUY      l88O                     bB q8C    y8#       ;     C88v                     :j98B  ;B9vl9888#; ;Bq  vU@8B8k.        , ;8B@  ;vYl:,  g8B@, ;   G8 ;B4   ;BT      BA      U8B:l#l            0BZ       9Bt                       .      r88U                                          
         C9           HB89                       Bcc8U   ;8By   ,;,     vB8B.                      q8B8U     28B8BX.  YBGLm98B8B82        :: t8Bv           ,B8B8; :    2D8;   Z;      #O   :k9B8p    Ve8BeCHrwjGgBp         Z8U                      ,      .6BO                                           
           v;   ;;;AH9Ok                        C8G8r  vB8r.         lB8BgA                      HB6L,          VqB8B8BB988888BD         VXcB8Z              CBcr89M     TB          YBBUN2eBBw  :       .rHbpC8BZ       rO  6B.                             98BY                                           
              ;v;..                             888r  9BA        ,@88By                       .HC,                v;y;vNBB8BGt          ymGBe                 OB  ZB8v   ;8v        92;Cq9; ;    ,            v  .t9BB8B8GvveB2                             cB8U  .                                         
                                               cBB; Lew     lHO8B8Oc:                         v                ;lv;;hCB8BCl           .c98O.    ;...;;,        8G   ZBDv  e0       gv  ,  v     ..          My            YBBG                             l08g                                             
                                              ;e8Uv@C:;C888B9B8m                                          .  .wcC88B80c;            , UBO;  lcGeBG8G6e8B8B8Dk  v8b   :X,vC8.     :O  Zq,   Be           :N6C,           l;OBt.,                           vO88                                              
                                              t8e9B8B8BOM:  ,8V             l                          ,    C8e8BctY    :.        ;. CB8b0eBpUrv:     :lV209B8; Z8k;,vv  66      j le8      .meBeZp#bZB8GH            j# 2B;  .                       .   cGBV                                              
                                              Z88B0A,      68M              v;                       ,.    ve8eB9;y.   ;        ,v  8B8Cr,                  VZ8; e8m.  .;BZ     e qT@G         ,2l;AOB8l           v#B0 .Bm                              vjeG                                               
                                           .  e8rHbT;;vkj8BD,                X                            C8B8BBHm.  Y           U A9v                        .m VB8   :h8c    gTbk#Bl             cc .HDGqZZOG8B8B88y  U8                              ;p08;                                               
                                           .  8B  ;ZBB8BGc;                  ArB                        ve8B8BDrc                  .                          kqb..B9   jB;   vtHZjh8;;         9B8v       .;lv. T: mB8e8B#                             HbeG                                                
                                           :  88                             rk8#                     ;GB888Gv :                                            ,eBC:   BH  m8     :0D tB OX.  yvGB8G;              l.  e8, H88                            v@OBH                                                
                                            .;8B                            .VZ8C                  ;O88B8ZA                                          .;rqCGB8GV     .Dh kB:     Z  BA vCGC8B8b                mD  ,B8    BeB                           meBH                                                 
                                            ,ve8                            ;cG8v               .CB8B8Be                                          B8B8BgjCbt,         M@.G; .  L  B2     HBg ;L            .@8m  NB8     4v;;                       ;,08O9q                                                 
                                            .veB;                           Vc8B               98B8Brv8     v..                                 6BO;     .             jcXt  .   Ce ;yC#9Zc    T0OA:v;;vk98BC    B8y                          .,;T9e8B8q  gO                                                
                                             N@8O  ,                        YpBe   .          B888H  ye    ;     .vm#eB8Gv                    v8B;                      BOt     v8  HBrr; ;       kNj#cBBv      #8t                        ;YAmb@tr,y, D0  YBv                                              
                                             vce8A :  ,                     k68M    .        l8.2BU  .e      l6B888G#mrvAZ94                 H8B; ,                     eBZ.   j8,    HH  v            Chv     U8Gv                                     8B   ZB,                                            
                                              ;Z8p ,;                      A88Z  Uk. ;        ,  9Bj   jjOG8B8GV           ;9e              98gv                        m8mN vBZ      ;8k :        :;9B,  T8B8gBBb                                     v,jB;   8B,                                          
                                               TBBAv0q:                   ,8B:   08v .           ,8B; .  ;M:                 ;8O          rB8G.                        .MBm ;       . jZlb8HkXpZee8B8p         H8H                                     ;; r8l   ;B8M                                        
                                                keB#.tr                   BA     9B.               8B;                          tC4;,vkGB8BB,                          .Y8q           B;  v ,vvv               jBc                                       , vBy    m8B8V                                     
                                                 y28T;Hvv;               V;      Bv  .              8B:                           ;0GeOjkl                              kBO          H8                        y8Z                                          eBv     HB8B0                                   
                                                   .68q,:vYl..                  gG                  j8Zv                                                                H8D          8w                         B8v                                          8BV       jB8BZ                                
                                                  .ckbGv ;    ;         :    . Ub                    B8j                                                                kBt         ABC.                        ;B8;                                         .8BZ0b.      re86;                             
                                                  X8;U0v                    .vv      v  .            :B8m                                                               p8U,        B9 8C                         B8,                                         98v  Vc9Ow;     ;l                            
                                                  Z8,.;Oq                   ;      48@.,              wB8                                                             , 9BA        v8v  8:                         y8b          ;                             .B8      ;vk6OUV                              
                                                 .B8..  hk;v;.  .          .      m8ck;                #89                                                            ; e8V        BB   8t        v                  ee@ p       9                             pBN                                          
                                                  eB  l                           8BAv                  8Bv                                                           lU8g;       ;BG   O0       OOB     ;      ,.   elV;m8       8v                           .8B                                          
                                                  j8v ;, ,                       8BCL                   98Z                                                           ;pBr        C8U   p9      B  8    .8:    y OB. DB  ,BC      ;8                            68k                                         
                                                  H89 : v.                      cB8Y.                   tB8.                                                          rg8  ,     v8B;   pO     Bv  Br     8   ;l  9B. 8Y   BC      9w                           V88CV:                                      
                                                  V8# V;                        G8c                     veBU                                                          k8B  ;     9Bk    be    AO   BC   . C8  AO   rB M8   ;BZ     .B                            6BX  vv;;                                  
                                                   B8 ,                        v8B                      .D82                                                          mBe   ;   vB8     @B    8    GO  l   By 4g    ;b OA   ;Be  v  B                            p8k                                        
                                                   CBD;                        O8Z                      ,OBZ                                                          C8t   ;.  @8Z     c8   vB    eO  O   yB  G     .  e     eZk8  y                            .B8Bv                                      
                                                   .B8j                       ,e8L                      .D8O:                                                         GBr   v,  8B;     UB   CZ    eD  B    8C        vvqGy.    88                                vvjBk                                     
                                                    m8Bv    ;:                UG8:                      .eB9:                                                         G8,   ,v lB6      j8   8.    Bb  8     8;vt6H;       ;9G  O8                                   jBA                                    
                                                     j89     ;V               k#8k                      .e8p.                                                         88     .;98;      O9   8     g0  B     B8Xy    vD;     ;8OBB                                    vB6                                   
                                                      G8V     vy              vOBl                      vgBH.                                                         B8      j8g       9C  HG     g0  8   v#0D q,     h8;     ZB8b                                     q8Z                                 
                                                       B8      kv            vlZ8                       j684:                                                         8B      B8r       Bw  Bv     9Z  B   :  8 jq       BO     HB8                                      v8O                                
                                                       bB6     ;k            M;qB.  .                   0eBv;                                                         88      B8        BL  8      O@  B      B  g        B8     O8C                                 .D   e8                                
                                                        OBp     2;           N,j8v  v                  .O6e,:                                                        .8e     L8B        8v  8      0e  B      ZA O         tZ    U8Bc                                 HB;y8Z                                
                                                        wBBm    B2           c.cBm  :                  AZe# .                                                        vBO     OB4       :B, vg      hB  8       8 v,          6   vBgBH                           ;BG   O8Bm                                 
                                                         h9B;   GB           m.v82                     yZe# ,                                                        r8H    hB8,        8  jm      v8  B       Gv,O           8   jOOB8                      ;    vB@  U8H                                  
                                                         v gB    8           N. 68   ,                ;vG8H                                                          cBk    j8Z. ;      B  GL      :B  G.    ,;0V .rB8X       B     lcBe                     ye    jB  9B.                                  
                                                          ; Gj   eg          vV ,8m  :                ;68j  :                                                        C8v    6BU   ;     8  e        8  Cv  ;Ty       V8Bty;  qA      ;A89   j                 Ge    8,.Bm                                   
                                                       ;hvZjc8   :B           X  ZB                  ;D8e  ;                                                         D8;   .B8v , ,     B  e        B; MccB              0B8e         .0BB  B.         Y.8    k8v  vB8Bj                                    
                                                  .q8B8BODkH2qGw. e           y   Bt                :D8B   .                                                         98    k8O     :    8 ;D        8V ,8B; CM:;yll#0,     N86        ,,8Bte8c         Te8     BZ  #8V                                      
                                               vD8B#Y           ,rBB.         ;l  ;O, T.           vC88    :                                                         eB    GBc     .    B,4p        Gp  B8tjB    ; vB9By     8B         p8  B8          gC     8@ q8;                                       
                                             :mBeA                  9GX        y   AeB8B8B2       .08B     .                                                         e8    B8V      .   e Dr        #@  8T       j  G :8G     Bh        0D  l8Z         rBb   ;88eO                                         
                                             ;Bc                      .GO       ;  .88BelOB       98BZ     v;                                                        BB    88y          G br        Ye  G        ;9 lX  BA     k       lB     BO.        Ge.  B8V                                           
                                            Vp8.         .               eG        68BO  v8C     DBD8     vw                                                        ;g8   .BGv:        ,g qA         8  Z;        e, B  VBN            B8      A8O.      q8UmB8                                             
                                          :kB8B.     v  .,rB.             A8y     B8B8    B8    hZ eB     h                                                         ;eB   ;8p,.        .O bH         8  y@         8 Gl  ,BB          .8G      mB@.      BBec:                                              
                                          8BHU8.    .ve  w8j   yckA         Bj  :e8B0      88   . ABt    vY                                                          D8,  YBj .        .6 HO         Gv  e         2A,B    BB       v  Bq      B8y.     XB8                                                 
                                         8B .CB.     NBBmBbvheB8B8@Uv    y   TvAB8Bb        qG  v q8    . ;                                                          #8y  U8m..        ;6 He         HZ  8          8 9    ;8A      ,  8B      8Bv     vB8;                                                 
                                        qB,  #8V     U8B8BeB8B8#vq8ccy    0eM 0B8B@          B8pv.8B   .  ;                                                          j8H  tBy v         g wb         ,8  0v         2CkZ    Be         B8M     q8B4 vc8Be                                                   
                                      ;88B   jBU     tBeqee@;     Z8tk,  CB8B8G88C          ,8By.UBO  .  .;                                                          bB0  28: Y         C.,C          B; ;Z          8vO    .8         v8g .    G9B8B8H                                                     
                                     ,8ceZ.  ,8e   ,kB8Hv         ;;mjU;G88BbrBB#4          6BU y28j  .  .;                                                          j8O  @B, L         TU C          @j  B           GVj    B          yB9 .      :9B,             .;                                      
                                    v8t t8,   l8y  cB8Z:    ;     Y  6B8B86k rBm  h        :8B  yZBH,A   v                                                           mBe  b8; Y         ;D:y          .B  g           Z;w  .  k8l        UBB  ,   t  8B:             v:                                     
                                   .8B. vBt .  y8B8B82;   ; .;       X8B82  8BC  ,;X       c8k  h08Cm   ;v                                                           kBB. j8v ;.         kvX           B  @4           q jZ               ;880 ; Ue   8B      wceB    r                                     
                                  j8B8v  tBy ;.rm9BDU      . :;      6B9   ZBl , :Ymc      gB.  meBp    U;                                                           V98V k8l :;         . b           0C .8           Mm8Y                 .qOqO8H   c8#      ..BB:  y                                     
                                 b8bkBT   vmjU8ev    .,.        ..vq8Bp   eB       ;jv   vmB9   @e8     H                                                            vGBk T8k ,;     ;kkOOZg8j:        ;8  B:           gBby;yCG:               vt;    88:    OC  8B. :                                     
                               ,;Be;M8#;   jkM:        v    .vV  .B8#    88   .     vr   ;O8D;; C8O    vv                                                             Z8U l8m  ;   Ov    ,y yO68Bl      gr vZ           .8O    MB              v lt.   Z8m    .8L ;8e                                       
                              ;yOBc ;j8tct9C,                   rB8;    8B           ;    BBt.  @e;    v                                                              #Bk ;B0  ;  8             CB8H     @  8            t9     ZB                c4v  V8BX  .eB.  e8                                       
                               ZB8#  v U6@C                    88j     yB            .    B8XT  bG     ;                                                              j8c .8O  ; 8t   VcyLv,      98B  vye  0t            Bj    ,8                ;Ll  rB0j6BBl    U8v                                      
                               qBU8p  hBC                    C8e       80.               A8ej98e8O     ,                                                              yBO ,G8   ce       l8B8pV    r6BY,88O  B             8X    Br                Y;Y;q8   ;       8                                       
                               DBN08OOBO                    BB.           .              8Bm  c8B8H   .                                                                8e  9B.  Z8vy      Ov.888;        @BU bl            ,e  CjO89               yre;9t          ,B;                                      
                               g8: rB8.                   rb                            jB# : vt888r  ,                                                                O8  p8k  q8        r0 jc28B        UBH 9             wHN:   g8M             ceH.V    ;    y6v;                                       
                            vhB8BZ t8;                   w                              e8  ;;;XB8B0                                                                   2BA VBg  ;v         8 vm ;B8;       ;BUj;          :wYYw#Ul.  m:            qH m          8O                                         
                           28BMLcOGq                                                   ;80  ,;vheB8                                                                    ,8#  DB  ;          #v 8   CBN       .qjq         ;v       ;:          A    OZ N          e8                                         
                         9B8j                                                          OB    Vvk98O  .                                                                  O8  j8h  .          9 6    ABD        yB:        9   4j              CY    et.j          0B                                         
                      .g8GA,vm                                                        v9k    vNHgBq                                                                     U8c MB6  ,          9 p,     Bg      .  Z        O;A;V8B             B     Dq HU      ,  l82                                        
                     OB8    m@,                                                       H;      vbG8b                                                                      8g  DB  :           2;O      O8,     , V,       ;B.   8B           HH     LB .g.      v  B8.                                       
                    ;BO  .ke9yH.                                                   ; .         0BBq                                                                      q8; U8C             m:O       D8     :  O.vB           8M          8;      8  mC       . rB8.                                      
               .  y;m8;  8B8  vv                                                   ;.Y         ZB8C                                                                      vBZ  68  :           mtA       eO     : v; BC          BO          B.      vG  j;        ,l#8b                                     
              ;wNy  cBv;8BO;   v.                                                  .X  ;       b88D                                                                       DB  cBj ;           AX0        BbpU   : v jB          8O          8        bm  M         V;;8Bp                                   
             T#Y    T8B8By .                                                       :M  l       ZB89                                                                       vBC  68 .            bO        p, H8  ;.  q8          BG         r#         :m.Cl         ,; .q8er             ;.                 
            pL      ZB8O   ;                                                       ;H ;v       cy                                                                          O8. H8H ;            gy    ryOZb      .  mB          8@         bC             m               vcOtUVyrv;lvvvv                   
          hj     :98B8c   v                                                         .Zc.,.     w                                                                           yee  e8 ,            yZ  v.   .e8D        8e        ;Bv         mD              4                  :vVy;,                        
       ,0A,     GB8B6.    y                                                          yk       ;c                                                                            Z8V HBm              2vt       C8c ,   0V,8Z       Bq          A0               A                                               
      cc      v8B9BZ     ,                                                           .9:      vr                                                                            ;ge  6B;              B             .  ;BB#0N,   ;D;           r@                .                                              
    A4        8B9Bq                                                                   Mv      v.                                                                             t8m .8@v             X  vvvk9B#.        ,#92Ahvv:              G                                                               
  ;A         88eBt                                                                      ,     Y                                                                              .e8  j8b              v       O8b                              B                                                               
.;,         8BBet          ;                                                                  M                                                                               pBO  B8v                      vBG                             O                   :                                           
           9B8BA            y,                                                                Y                                                                                GBy HB9                       :B9                            My                   .                                          
          OB8ev              v;                                                               v                                                                                ;BB  eBL                       .B,                            O                                                              
         #B8k                  ,          ::                                                 ;v                                                                                 r8B  88                        Bv                            C;                                                             
        DB8;                                                                                  ;                                                                                  c89 ;80                      XB8y                            e                                                             
       OBg                                                                                    v                                                                                   c8C U8;                v;vhg. y8B                           MU                         .                                  
      8BD                                                                                     w                                                                                    c8w 68              Hy         gB  ;   v                    G                                                            
     p8Z                                                                                   .  :                                                                                     28: B2            U  :lUjv     v#  .                        B                                                           
    #Bj                                                                                                                                                                              pB yB.          .9 v  Y88g       .                         ;O                                                          
     .                                                                                                                                                                               ;DZ.DO           vhcMZ8m BB                                 VG                                                         
                                                                                                                                                                                      v0CUey        k@;C889    B...                               yO                                                        
                                                                                                                                                                                       ;p#Oe        BD @BC     8M;                                 ye                                                       
                                                                                                                                                                                        :TqBV       89vBH      Bh                                   ;B                                                      
                                                                                                                                                                                          ;GB       j8BB      @e                                      Bv                                                    
                                                                                                                                                                                           ;8p      YB8;     VO                                        kc                                                   
                                                                                                                                                                                             6T.    q8B    ry,                                           H                                                  
                                                                                                                                                                                              ;   OeDeO::;.                                                                                                 
*/

/*
HHbqG2HcbZDO#pC9OZ9mybqHctpDOt2ZbCbkO#m299ZO02C@ge6ZCCGZbOe@0De9O6e@96g@6ZqCe9qG9cOOO@Ge02OGp0ZtOB8jGbj#pDGO9DOmOXt9U@ZUcZ0D2cmUU8tUNLhwA;cwcqbyU0ycp4MX8kcCAmmchttvUThrqkmmk26wcjrvU#k0jlclUUXyrO8tr;GlOkT9rqc2OkvUXbU.vvecG2rk4ckALY,:Avcjyvclmr:Tk.jyj;v9,Vvvy:m;,Av, lY.,v;  vbTm, vBZZtp@DZ6DODOZODOZODODODOZOZ
tmkj2CCcHCj0c2jbkt9GtMwmtb4mO@X0OjtpmZ0kmq2#tm4Hcq96jHt##CCOOZqCtZOOggO#Ccmtj9OC0pC#C@D9t0ZqZp@pCeeZkmAjkZO#CDbHtTyOkm#Xrtm0jh4rvjCCwLMpr;vyYZXTylvMvHkVNm;tYyA4MvUvvvvrkXyjUc#HMwrvv4H4mTAhrAvA;wZOY,vj;O;XyVcH@tvkkvM.v;jUyZvUXVvMv;r;X vHyyZv.;,lv kv;;vm;:v;; jv Y;; .;  :.  ,Tv4V .692tUCb#DqC#0#0#0#q@qZ0Zq@qZ
OZZCjCeZtC#ZCpOZqcjb9OtH2ZCt2C#bZqqCjcDqpHjOGCjt2COeeZ@pZ#DO6GD0qqO9GgB6ODb2qOe0ZqZZ#9G6Z99pBqDDqOeBcckcjZOqOZDtjUrCOtGjUHG2ZcANLyCthkkHXy;AvT#kjlvmUcCXw#rvDLLNmMVwvYUXvHUHc42ep;lkrykXHbwpLwmUvvjkjl;U:AOlH;CUbgj;pvkNLvAtvDbr0UvyAylwH,;MLlwZ.vy;Y;A;;MT9Xvmvv mL yV;  Vv;;v.  YVy2v OBDCt0#Z9ZOZODODODODODODOZOZ
@O#0qZ2q#GC@jZ#jtZ#jkCZHrq#c2#m@Zq#ZqO0e@HpZOO#Z0qj#9BB9CCCOGgGeOD@#ZOO8Z9D#COGG0Z0DqgOe6GZpOObObeO8DptkqOqOOCDbttkHOjjCkrtOZ@hXTwcmAhNUr4vyLrmVttvXXc#VhXB;w8YtUcv4hvrjAvHUHmUgZX;myvNc;jkvvcUhrvLtBH;V9.mwVVMmkZ#v;eAmAUvbrrqcVAVc;L;ryj;AyUvpy:Tv;vv:v;VBvllvl,Av Ywv  XUv;v...vh;XM mBCZc@0DOOZDZDZOZDZOZO#OZOZO
C#GqDOq0CqqZOjtC00GGGb#qtM2Zqt2ppjC2j2qCO9ZcDO69O0OCq@eBeCCO99gGe9O@Z2#66ZB9bjOZ@@0D9OO6BG#bGDqqOZ9GgcZHb#OD@b92mHkrDmwDbhyk0D0kMrm#jUMjkwyyl;Uvv#UvyXGAvvmZlmcLXrUlmyvlqMTDhc2bbck4M;jZlvZVAvjmcrwvHBh:bDlvXVTHcq94;mtvVj;vcUUA2rYpw;MVler;lHCvC;yHLvmyY;YOv;v;Y:;v MA:, Lry vv; ;ALvr hBZbtCZ#DZO@D#Z#D#Z#D@Z#D@Z@
bbbZ0O9DtZZZO6@DjHtZO#tCjHHbqD#CmjtHUtCqpOOCqZ0ZOgOGO@qGG8OjC9GeeB66q#DGOOOGpC9gOqqO9DCOeeDZCO9Zb#O@#CtcHD#OOCC#HNkA2bMmqmVhXZ#cmAA9cmNp2cTyvVXU;Nc;TyyCAV,q0;prYwhwvTvvyCVZmUcjOktvwMyqj H0v:ktV;NDy4h;vGDUyrUctp#twlDvlmy.V#4 w4UCm Z0VA2;hAm:MA tN;yYv.:Ov;;vy;,v A;.. .vv.vv. ;vlvV y8@@mC#ZqOZD#Z#DZO#DZO#DZOZD
2jqbctO@C9gqbmODOZ0CODgGZjpHctpCOCbHU2DOCkCDZCCCZOO@Z9eOOgeOG0O9BeBeB9D0ZG99Z0GG9Cbj9G#De99OOZGbmOBZOeqjcqDZe0tOOMUNt#crZ@rVyUqj2Ty002hAejA;wyHONUtr.y;mBUvv8A;bMwwy;vyVlNm;ctcVjkt;vU;qC Yer:MXYt.kDy2U4rOyHvrkCHC@pvtkvtU .m8ylcvMO;cplYj;rVtvLc vM.vkyvv8y.v;;:vv ;;v  .;;...; :;vvA VBO#j2Zq#ZD#O#DZOZD#OZDZDZD#
jCtjjpcptCCGBgOC0gOZCqq9O9ZCtpjC@COgHUHt#g#ODZC#@D@OO9O9DZOB99@ZOgeee8gO0#OBO@Z9COD#jDOOOOO69DG6jDee#8@jCZOGZO0j0cckwjcrkZckUwj0btrAcjkvHjjlYvLUh;tU;MVl2Htvk8vrOlMV;YyllVkXl#bmvDqcANhmj;rcU.;GMvlrUrjcVU2vlrlA@cjDkXvq.Cc, ve0XTVlUAkDymTvvhMp;Xv Nvvt4ALO: ;vv;vv v:v  ;v;; ;,  YV;r: 8BDtC#DqOZD#DZZ#D@Z#D#D#Z#D
C0qCqtttbbjH09OD9#ZqOZZC@DG9ZjbCOCj0g#jTcOZqCqO@q0ZZOZODGD9GDb9OOOG9e6Be9bZ6BZCOD09pt#eZGG99eZ#Z9bOeG9ektC9D@ZOpjttkc4tcwj2kcNc09btyA4CrvMbwMvYYmvl4#AX;jjZUV0O;UlrAvvAVvlvtwyDmrmOHUyXw#;ybH,.L6:vhmvAc;wGk.UhyCbkjZr.ky;qv,;HjpvvAtlyZtUcwyvyU;Tv qy,XcvVw., Ac;;l;v;;  MAy:.;, .:l:v; Y8OttGDZODZO#ZZD#DZDZO#D#D@
@G#Ct0C@mjCpkbbDOGOOqZDGDO09O9OjcDCHmCOOmt2ZC#OZ#OZGOZ09ODZGOZ09G9OG6e68eG2Z6e2qZDDqpOg9Og99OOtp@0C9gj9Cc0D9#9OZtjtjbNHjwjjtkmAc9GqHVtkbArtUrTvNAH;NClvtyjhwHv#C;UAMvvvLvlvvtMAjpYbtUtMwpyvNHV; pX.lcVAHv;q8;YCrmCUtG4;jj wr:;ApXm;vrAUOAwCjtHHN;4V kk,rHlXt.v lN; y;vv:  jZ;. .l; :vv.v: h82tZDCO@DZD#ZZDZZZO#DZZZD
ZqOOqO#t#j0pHUqpjbDZ#bCb@Z9ODO6Z#COD#bjjjttC9O#p@DOO9DOOZqZOgOZqZGeOg9eB8e#29eGO9ZOOqDG9G9eDO9OjZD@C6C0OqmC0q@990c2Uqk4cNNZpkmkrc6GhTwLUbVkmvrAvhrMMAwMcyvt0Thle0YpATvvvY;vywjkjCrVZmcjlMHc;THvl:rvvryc#A ,9k;4rwppH80;HG:lk; vGAvHvvvVCjAh;MMYArXm Lm;r4LAklv;vUv V;:V.  OM . ;Hv.,vl;vl  9etZZZ#Z#Z@D#Z@D#D#Z@DZOZ
@Cmbc9C0Z0m0Zpccktjp@Z#OOOCG9ZZOGq06Z@#GtjjktqZZC0DZ0geGOOZODZqDZOOeGG9eeeeOZeeeOZ#GC#O696BZC69qCO0#O699ZqCOZOZeDDOpUmYjcwjDjHcNUpBjwAvlHXYmYYAlYrlvlHtHyvwGcyvvB;lMLY;yYvv;vrbZtmvbjhpLMUUNVbvwvvwAwTyC4v,v@;;Hvh0kmOYv6N;8V.;8k,v4:y;r0HAYTrlyrYH ,cvMvmrLckUlHM :;.Yv:,wv:  vyv,.;;v:Yv M8Z#OZDZZ#D#ZZD@DZDZO#Z@Z
9GGZC#CbGgO2Z#OCtHc4Z#Z@Z9Gq2C#DZqO96OOOCtq2jttmpCOZ9OOZG99O9OOZ9OGGGOGGeB8ObDBegC002#O#OOG90CODpZD@#9BjeeGO9D##DCqZUmhMOmwt@HUckAbGyLVlpwMycVXM;VMv;lcUXY4jbvm.mb,vlyvvyv;:y;AHDtt2ZvHXrkYXvHHvMvUYVy:l2mVvNZ:mrXOjX0jvXpVgkV:Njr;yvHL.U0t;Hwl.v;4v yrhAkwvYT#vyA .l.yv..Avv .lY;..;:vvlv .U9Z#9#OZD#D#OZZZZZDZDZO#
jZXjC0qq@@06eZO6CpC2HUm2cpj09Gq#0bq6eOCZ#ZZDpqq0pCOGOGO9O9ZZZZO9##@O96966eeeDDOBBOOZt#C#Zq@99ZZ@C#CqCZ9G9ZC9GGO9ODC@ptCNU@kH2b4ccHbGCHllc2k42myc;vtCyAwCHNyh@@kk;9wvrrY;llvvlX;cCbjMGjmc;pmAvycyr;H;.vv;HUvtv8.vCAGjLjkXrUwyt;lUXpv.;Uv;VjrljA,vcvlM ;McrmNm;wAHYr,,V:yv  H:  .hV:,; v;vvr;..mB9OO#DZZ#D@O#Z#Z#D@ZZZ
qpttjkqZC@#C9ZZODOeO92jHmC2mtCOgDppjZOGZ9O@@@#DCC2Z#OD9O#qGgGD999DODGGgGeGBBeZDe8eeO#0OCbZG#GgeDGDZCDqZe9ZZ#GG9OGO#jOm0mYtqtjCCkqUyk8ChvYrXAM0MhN;yCpkrt0Nvv2@hcwL0,;yTvvvMYvvVvpCcy4Ckc;UHwXyATY;Uv YvrykywUCm mXqcNkHmcyrvmAhLlUA;vYmVyjkrVh;lUyvv .l4MvUUMX;Vkv,:v,vv vr., ;r;,:. Mvv;rX: vBOZZZ#D#Z@ZZD@ZZD#DZDZ
9ZObcmmmkZZ0Cbb#q9e6OOG9cktCmHc#De6#t2CCZ9q99gDCctttmp09GGO9OeGeBB6gGGGe6eB8eGOeeBee9OZ9##ZG2e9D@#ZO9ZjeGeeD06OOOGDj@qt@yHZ2XtDHbmvA9tUXyhNvytkLky;XGt;vB@rrUcmb0,vT,vXXv;v;ylYvv@CH:2hmVVlUjrVXMcvvvVvtvUhvV:O:;AjDhc04wl;rA0v;ktUh;YMrvchHlU,lyrlU  NwH;kVUkv,H; ;,:;A ULlY.;r;v .,V:y:vUv. bBDDqO#DZZ#D#Z#OZD#OZO
OGe69@OjcqZHb#Oq0CZ0Z#99GO#kc00cbbZ9eZ@qHNC@OZDDO#9DqtCZG2tjZ#DZOD999O66egeeBGG9BeeegDqDOCOG9q6OGtpZGOOO9#D9O#eDZOGCpGqcqMjDqHDbmCyk2OHkrykAXktAX4;vZjv;2Gw4rhkNprlbV:rhvvv;v4AyvMqCkmmLTyLvplwyyNM,kh;Tyycvw:Ah.vUqtA#NvvrMY@v AbUV:AyvvmkNyy,vl4UU; yUm;kVpMr:4Y.v..YA.;wvv.yv;; .,;;L;vhy: lBgDZ#DqD#ZqZ@Z@#qZqZ#
2q2OOZ@D0@q2CtktjZ0tp#0bbGG9C2p@tccC@Z#8Ztptjtm#9GeBe86q9GZZC0pC2q@9Z9OG96eGOegOBBg8eg#CZObO9OO6GOt@#G9ZOZmZBeGO#9OO0ZGc@mAZ0mbGAkcmtG9Cv;rZcHqtlkylN@HvrBtTVvkyvqwVm;;rlrvvvVhkrrTjmC@UhyTypyUAYvVyHt;vkMXvvTv8:;AtHmU0LyTtvCV; CUXvA;YYrybTy;vvcALl vUm;vAUAmvVv:c;,kc,.,vv;v,,  , v,r;vUTV. beZqZqZ@Z@Z@ZqZ#ZqZqD
Z2#0OD@9GOgDg@qp2b#0DZ#C@0##OOD#Zppjtkj#O@99eqbC#COZD9eG9Z96eOD##0Z@OZOZOZGO99eO86eB8eBO@CZZG@CZOOOtCG9@DOO29BBG9ZG9qbZ002ckHHHbclONtOgCHVYy9ccbMAtLVUcMHUeAXvltrkkvrvvlMvl;V;rlkHcUmNt0ZcHvwTrkv;vvNt;:bLUv;ryGH.MccHXkcv:rNX2Y Mb2yrwv;;AccAy:;VMrv.lUmv;XUvUTl,.M; wT.  LLvV.  ;  l;V;;mHV, y8ZD@Z@Z#Z@Zq#0DZDZOZ
#CDCbZZO#OZqOgDZC#qbbCCOO#ZGZ0CO9Oq@C0ptpqbO6e9O00t22CCOGO@DO6eeG6GGOOZ9OOZG9GOeeGOeeBeBOOOZ9GbOOe6OpOe9q9e9CGBB9OOGOC@qmjt#D#mybUk0wcGq2#yYcZUCjyHUvVh#Av9erUvhppCwV4Y;yywv:LvvLUHXHkACct4VAb;myywvAm; qkmNvyYHt,vcbmyHHUU;wycX;lptvUmMvvmkHyv..ypN.;VUUM;jh;vcl. r; MX,,.hrlv;     ;,y,.m0y;::G8qZ@Zq#0ZqZ@Z#Z#D@Z
jj#@DO699ZDqD##ZOZGOZq#0#qOOGOZqD960#OOpjbqCbjOe8eB6G#@2qbZ##qeB89OgeeBeeeGO6GeeGOOOegegeG9Zg99#ZqOO9#9egDgeGOeGGOOZeZb2@cpC#GeTcgMqmy6B6bhMUCXrmUXHvycMcykBwAwvjmObrrHvlyry;VyvymkUbjtcUHXTU0;lHY4AyNY.cmUyvVhHj,;NpUHAtwqvyrccUTbrvXHkL;HyU.A;;vjUy;lht4,Zhv,yV..vv NM.,;Ul;;. ..  ,.l,.X2M;v.meO@#@#q#0#q#q#0#0@0
OOpjmtjbZG9GO9#CbCcZG#pO@22Cj@OBOGOZCO99ZZC2j2p#ZqCGB8BBOG##O9Z9GeODZeBB6Bee6eGeBBGGgeeBgeOOOGGGZZOOO6gO98G9eege9GOODeCq9Zc#H0O6UB0XDhjB8pXmYXckm#Mmy;2Yr2vkOckvA0mOUYwUvMlA;;TvvNHHbmhbCUAk4Ok4X2mM;Uvvv#MV;AwNtY vbccVpmclVHmpyvOhAVT4A;yvc.V;::lHm;;AHk;pyy Yy;,:v vH:.;4VV;  ,   ; ,:;Vcy;v:;09Dq#qZq#q#0Z0#qZq#
C9GDZ22t@0@#O99OOjtC90CZOCZOOqqtDO9OOOGO#jt2ZtcjDOpmCO6@#B8ODOGO699#OZ9e8BBee6GOBeeeeGBe8ge9DD69e6e9G9e9OeBOGe8geGGDZg9C9OpbC2ZGtU8hccAC6@HcUXUhmpUAqvvr4OyyBbUwvhA4mrYNNVwyv;yYvvXc2phwcTvvrwq4A2my,rvr:Ccr;vUmcr;l0kjYppHvlcXry:OyNMAyNvYvqvY..;Hr4;,vHNvCLL;vv,,;v vA::vvvM; ,,   v  ;vyjw,v;,46ZZq#q#@#0#@#qZ@#0
cZD9GgOZODp@Z9#GG#qZC#@#@#qDOeO@C0bO9e9ee9ZOgOC#DO@OOZqDZO6GqDGGOeGOZG9O9BeBeBG66eeB6eGeeBeeg6999g6BGODgDGeeZGB8gG9GZ96ZjOO00tjqjU2qrqMmOeqjcHwNUctLmU:4bqHytpUbNMvrHH;VjlNMvy;TVyVmtCH44tLvrvcArtUU;TNV MZU;;AjU4ArtccUHZtLvHrVM;jr:hcMTvlyCcv. yDAjr:VhhVOMyvrv; v; rr.;:;;V..,.   ;  ;vvCX;;v,rGO0@0Zq#q@0#0ZqZqZ
90@#ODOb9OZq@Cqb0@O@#00jtcbC#qZDe9GOZ0#Ze6GZ69GGgOGDOO9ZGODqq002ZGBGC@9DGGBeeGe6e6e6eGBeegeee6e99Z69geGq#Dege9eBB999O@g9CC9##b#ZCjcO0cZcOe9mqwTHmUHHrpvvHyGNwmtH@AvApULvbryAyY;rUlYwHmUXccHcjVmUjGbh;y0: vejv,vywcM;UttUmZmAllUAA;Tt;kOVv;yVpm;  v6Utv;kylyjU;:r;V.vX;cv.;,vv:,..   :l  ;,:pH:vv,VeD@C#0Z0#0#qZ0@0#0
C@0#C0qZZ69#0GeGC#ZD@O99#bmccCqZ0O9B690#ZOOOOOZG9DOe9O#OZODOO9ZDqD9eOZ0G#gBBeegege6eGe6egeeeBBee66ZOGBeGqC#BBeOeBB66O#@e9@#Zj0@9ZOmc94U#28ecjtXkjjj9HkhvhweCMCqcbH;L4jUywtvAlLvv2lvwcpcNkHLUpyXkH@cb::eY vOkX;vmymv.rNrpN@kXlMkvVVVCLh4YTyllhGv,vl4rN4vHrv;tU;;NvN,Alvm:  ;MYv     .vV  : .0t:vv.reO0@qZq#q#q#q@q#0#
OCDqC2q#ZZZ0OO6D#ZO#ptCCODO2tkj##C#ZGGBeeOD##C@D9#6G9OOqD#DZOOG9O#9ee9COOOBgeGege6eee6e6eeeeB6eeBggZD9BeBCcO8eeGeBB96ZqOBZ##@C#0#Z2Nkbrb@Z8GC2kw04tbbAcHTwtGhhCt2bwAlHDyvbVYwyr,wrvYXtckUHtYmAwwvyHj4,Zy ;j:vv;UTyV;lhycUCjkyMXyvMHwkr4;4yvvHeV;mvCUHcVw4l;mkv;c2Y.HyNk ;.;Yv:. ;   A;  ; :jj,vv.ABZ@0@0Z0Z0Zq#qZq#q
e9ZO@##D0CbZq#CbCGeBeOZZp2#GOOq0q#pbqOqZOGOeODC@#O99GeeGOGG6O9GeOD#OGBZ69O6BeBeBGe6eeB6Bgege6ggegBB6ZO9eee#t28BgGege96ZZ96CC@ZZOZ#b6AZOcjkD86G4ycTMc9jwqhAcDmMmCtpmhYk0hvTmvyA;;v@VrvH2mMTmUkt4j;LXw2;MHY CA,T;cOhAyvjvmUCCmyArk;kjVhAwvVAv:rOMlLvZHccyVAV:HNv,Djv kyHM: Y.,;. :. :.;   ;,;0t,vv AgO0@0Zq#q@q@qZq#q#
cOO9OZ9#DZZC0b@pbCOZGZG6ODtjDZODO0bcbOGZ2t0ZC@69OZO@#D6GGGeGGOGO9O9D6GGOO#OOG6egegegeeegeeeeegeeeeegeGe99e8Zt#8eeGe6eg6#O6O#Z#DZODbOjw8jcjCege2wctmjO0wcCy2tjwAkkAkwAU2MyvmlYUvvv0pyMybUwXchAHHqwUryZyVhN ZZ AyMZrYvApvVUUCmAVcm;HthXvTVVl;:yqtyyvqHqkTlmAVHyv.pHv:YACv;.v.,v  .  ; ,   ; ;Zm;A; MeZZ0#0@q@0Z0Z0ZqZ0
m0bDOOZq@Z#Z#ZZ#@O0pb#ZG6eZC0Cc#@OODbqZ#@OOqtqOOq#@O@CpOO9O9OODODOOO9gO9OOZOZOOe6e6eeegeeegeeeeBeBeBBBeBGgB8OCGe6e6eeeGO@9GGqCb20BGZBjCDyb2D#8OccqUkC@kp9Xk2UhUcMrhbhUc4vvAhvkyvrADrkYhpwHpHyAU@cCyTCmlNAvM8 4vvkYlvyjTVAymcwLmmvwrMLyAVyr;;yBpyYvkUcwlvwA;Tvv.tXv;wkU;v,v:,,;     .,.  . YOVYv  UeDq#0@qZqZqZq#q#0@
9Z@bOOOO9DO@Z0D#O9GZDq0OgbOG6qbjpCOZGeB9DOG9DO9@ZOOOgOOZOZOO9OG969e6BBBgBeegBGgGBeeeegeee6e6BeBeBeBe8eBegGG68e#ZBB86eBBZZDGO@C#Z9#9bgbXbtU#pq9ej4rkHOOqUDcMkhkpU4Lyb2YycUryHvlc:y,lrAly0MAkm4yNtkmMvTh;4v::e.whv0yw;vAj;NvHpjHjNLyyXrNwATrmvveqYylmANyV:VT;UTV:bH;vOM:vr ;;  :Y;   ;:   :.HO;;, .mB#Z@Zq#q#qZ0#0#0#q
GOOZ@OZGO99OZDO9ZD#OCO#DOO#GDOOOqOZ0p@ZBBeDGOOO99eeeOe6O06g9OO9eee9e6e6egeO66Bee6ege6eeB6eeeeBgBeBeeeBeBeB6gBB96Ge6BgBe6DO6eZ#qOZ@9eZ#mjBpHqC0eGhvwbC#ZcmCAALcOcHVhO9MlqmVlryvUvvy.hUy;pkANm4MTkUHkvrL.Cr;.0vVLvcykTvVtvwrkmUhbtXvvkVUMUL;UA;BjvrLkkMvv;Ay;Zvvl4y;vtv:;y ,;; ,h:   ;,   ;;tt ; . CgZqZq@0Z0Z0#qZ0Z0#
9GeOOOO@OO99G99@ZZO@OZOD99OO99OZZqOOO2cj@@9gBG9D6GGG6G89OZeB6OODGB866GeGeeege6e6egegB6ege6e6BeegegeeBeBeBeBge6BeeGeeeeBegOgGG#bqDZG96GOXOZUZ9pgBtrrpjC9pXcjLvyOwmwUcjUymjyXrklMy;jvY9HvVtrmHHkwMtybvMh;j0y:UATL;HVmjvvcVAw4AwvcXk;;HrAXyrvkv;BUvcy0tA;l;rvl0;rATr lk;;vv,.v: ,v   :,    ;YGy : . OeCqC0b#0Zq#q@qZqZ0
O6D9Gg9OOO#DOGZ66OqD9G#ZDGO9ODZO0OZO0ZG9Z9O@CGB6O69gOZOBBeCq689qpOB8ee6ee8ge6ege6eeegege6egeeBeBeBeBeBeBeBeBeeGe6eeeeege6GDDOGCZC@0OOGG2mOHCOZjGOpUX0bDZHXZh;YjtVHwUkCYy0cllMAAkvAU;DGv;XXwUNwywHvqcym;kOy;YwZr:yHAp;vylvLMUHrttwAlAljAvrrcv;8vVy;HhVVvvVvlAyVLc; mv.vlA: v, .;   :,    ;ke;. , ,8OqC#C#q#0#q#0@C@q@
9#C@9ZO#ZOOCC@OO90OG9D9OO@ZO9bCZZ#OOO009B9OOOOGDDZe9GO@Oe88ZbG8B9q9B86G9eeeGe6e6egeee6eee6e6BeeeeeBeBeBeBeBeBeg6BBB6Bee6eeOOOOGC#qOOOq#ZCZ@m@OtmZc4yOjq#0mZk;vH@lYtjbGtVpm;;r44Ul:H;rOVvlXhwUhhMjwjCkml;8wv.X4l;vmlyvvyrvvkUhYjHkM;;rjVvHYmv;Blrr;UMXXM;Y;vwb;HV.;Z::;vUv v;,.;  .;     :#G:. . me#q#C@0qCqC#C@0@CqC
O9pCqZbZZD#99ZCqpjj#OggBGGOO9OZ9OOZDZDC@OG9G69ZgGGO#98GOOee8gtCBBGDO6B6G9eeg6e6e6e6BgBeegegeeeeBeBeBeBeBeBBBeB6BeBeeeBeeeB9e##9#jqZOOOc@qO0pXZGHpGcMh@HZZqtmv;YphvmthDqy4j,lyNNyr.rN;#yV:wNhUUrykHHjkcy #Al tvAh;tvvyVvrVvjmVVMYvUV;rjvvUytv;eLHv;VA;vlvv;;km;HV;lwv;:.HV Y;, .   ;    .,B#;    BOC#0qC@CqC0C00q0@0#
O9GZee9DGGO@GOZ#OZOqO022DgBD9OO9e96#OZ0CpcCOGO9GeggGgeeO66GG89ZZGegO9B8O96B6egege6eeBeege6eee6eeBeBeBeBeBeBeBeeeeeeeBeegege9O#ZObCGZ9CqZZCcCtHBZc6Ocj2bZOpccU;lykAycyj9UmcyVTlp4MYvUvctv;;TAtNAAwMtvyNU cUv 8vHT NvVrvvvylmkrX4Y;yk;Yqv.yl4vvcvm;;mjAvvvM:vXAvpY;vr,;v;cl Y;;     :  ., LBX    l8p@C@C@C#0#0q0#Cq0#0
qDOO9eOGqGOZOeqO9O#OZZC#ZGZO0pHjD#CqDe9BO#Cj#Oq0@9OG96GBB8GO9BBeOOGB9eeGOeee6egegBgegB6BeeeegeeBeBeBeBeBeBeBeBeBeBeegeee6eeeG9O9ZpD6q6DOOZtcjH#O4tB2C09Z0CHjcyrrvp;XwmObmjtYrvcDvmvUllgvv.vhHTAXrhbAyTj;b#: 8mVX vvMTl;vylVkhhkyyTV;LDv yAAvvTLq;;kUMllVkYvmAyjv.vl.lLvpy v;v    ,.     9G,    ZB0b#Cq0@0@0@Cq0@0qCq
O#9#9999OO9OeOOZZqOOGZ#C#0tjbOeCC2cbDO9eee8OCD60b@0CCp#9D#6G9eBgG9e66G6Gg6eee6eeegegegB6e6e6e6BeBeBeBeBeBeBeBeeeBgBeBeB6e6BeeOODGC0O2GBZqb@Ujk2q0AOkT2ZCpcjHhXhXvkvMmNO0HmprhvrGryVTA ZUy:;4HTAAhymHwy@Yb6; cmvH ;YHr;:vr;vckvmvVmcvAH;.4YvvryHt;;wy;vvycrvATlcv;yA;wv;Ml Y;l.   :     M8r,   yBGq0qqC@0#0@0qqqC@q#0
O9qD99qOD9Z96OO6O9ZZO6D#002bC#bCOeOO20p0pZeeeChZ88OZCCbOD@ZGO6966eGGG6GeGe6e6e6e6eee6eeegB6egeeBeBeBeBeBeBeBeBeeeBeBeB6e6egeggDOOOqZC0DOZCjCHwmOe0ACXkcDjC@jhmMHwYvrrAO6HMkCUHv0CVArU yGvy;XMwyr4XAcMVcLr0v M2:b:vXcmAvvUY;ckvcXT@X.rcv,T;:TUwjm,yUwvLvXr lrVy4y;Vv;.;;kl lvy:   v   .,OG;v  .G8ZCC@CqCqq@@@q#C@CqC@
GqOODqOOZOO0ODZOBO9O96ObZ#OcCO#j#9BeG#G@0q#bGOb02#8BeG6OOO9OG9G969G96geGe6e6egeee6eeegeee6eeegBeBeBeBeBeBeBeBeBeBeBGBeeeegeBBe6OZ#b29C@eOj2tcrTtZqhpqyyC@20jHMkyUll;ryH6qy42tjyApVwvj;.9Hr;vTrrrMr;4jyHLAmv yB,U;;MVXMl:hmrrcYUAhCp yU;;A;lXttpv;kywVvlAY tHryUY;v;v ;Vky v:l.   y   .;Bc;v  vBDqC#C#CqCqCq0Zq0Cqb0C
9GOqpZZ#Ce9Z#OZqbDOeeB#pCZ0Ojpqj2Z#OG86BBeOZ#Z#GO2OgeBeegeGG99OG9699GeGe6g6e6eeege6e6eeeeBgegeeBeBeBeBeBeBeBeBeBeeeeeBgegegBgeGgDOCZDCm9gDkHG2McmgCkepmjbbc0jHmrVyr,vHNO9MrNchwmmY;vtU.UCU,;mkyArU;rjrMrhOv :8YwvvlvyUM;lplYqTyAvXH.VrVrlVvrcOU;v6rcvvyAv.qrvvAvYMvV,,vwv v v; . v    A8yv,  jeZtqq@0qC#Cq0#0#C0bqCq
GOGDZ@OO9ODO6j0ZObjcZgBOGZ0OO@ZmtC#bCq@pZG8B69G@GO#ZGGe6BBBGeGG9GGG9GO69eGe6ege6eeegegBgegeee6BeeeBeBeBeBeBeBeBeBeeGBeege6eeegBeGG9DO2tcG9#p99cY4b6rtZ9jOjmCq4m0H:Vv;m2jBmMycyM4qC ;Ar;w@Ur,mqArUXYYtMwlMeV,;BmlyvvVXUV:Vw.vGTwml;4:rMAVvyAXm#H:YOhXvMMk;;Ovv;MvUAvlV.vy; v,v. , .   ,9el;  ;eGqZ@@C@q@C@q0C0C0C0C@b
#ZZGGGOG9OO9O@ZG9OqqcqDO9BODZOZqj2pCCD@ZjCDOD#Oe6GOOO696eeB8ee6e6eee9G6eGeee6e6e6ege6e6eeeeBgeeBeBeBeBeBeBeBeBeBeeeeeBeeeeeeeegeegD9DC0qpZZ#UeOAAh@Zm0tCttkjZpyb@v;kvycpOGU4ttyyme,yTH,NBUUvVZMNXAlvU4rL;#r;v8myAVUrHrvYlp;vDkMryVj;;UM;vlhcZcm;yHXATA;k vgr;ly;Y;vv;.;T; ;.;. , ,   r8c;. .ce@0@ZC0CqCqCqC00qC@C@bq
qDeGgG6OO6eDO99C0CG6GZ#CZZ@Z69GGG969GOG69#9ZOOOqDOe669G999eeB6e6eGe6eeegeGege6ege6egeeegegBee6eeBeBeBeBeBeBeBeBeBgBgBeeeegBee6e68e6O9Gt@Dte2Tc6hXjU#DptHH0CcZb2kqAVMV;c0mOcrc9tNMOcVkHl;OHyHvXVrArUvAkrv:mt;vGjHc;UVyvchLjlvDyvvUyD;.r4;v,VOBykrmvAAUv;U AOV Vv;r:y;  ,Y;   v:..,;   m8V;  l8OqCCC0C0CqCqCqCqC0C0C0C
Z9#ZZ99GO9GeGOOZjC0@CqZBG0C0tCZ6eO#999OGOOOeOG9qpq99ODD6O9GeeBge6e6e6e6e6e6e6e6e6e6ege6egeee6eeBeBeBeBeBeBeBeBgBgeeBeB6egegeeeGeGBGZ@60jOt#60HmZTjkjCjmtcCbC2tmymmvvMvNGcO#AAq22Lt2Yv4c:vBhmYUVXhArvlmkAv0H;v8qYUlUYyVc4vHv;CUvYMv@v.wv;v.v8GMcAL:wNv;vV,qm;.mvVr:y; .;Y    v:  .;   9glv  bB#C0C0C0C0C0C@C0C0CqC0Cq
99qbZ#OOGqZO6Gge8ZC0ODZqOgeDqjjc@#DZOOOOeOCOGZZOOC00OO9DODG9eeeGeGeGBeeeeeeeeeeeegege6e6Bgeee6eeBeBeBeBeBeBeBee6Bee6Beege6egeeeeeee9O0OjZjUZOkrepNcU@mtUbjCjbjtTm0yyLTltmC6UUkmHcAtY;wCl:ObtyyAAHyyAvyHHlpH;lBZ;kvp;lXcrvHv CUv;Avj:.y;yv yBjypAN;kr;;Av:tT,vO.v;,h: ;Mv   .V   ,.   6ZV. ;6OCqCqC0CqC0C@C@C@C0C@C0C
CO99OOZZOGq#O9Ge6ee6O9Z@bqc0BB6OZ@tj0O#G99O02OOO#G0ZOeGOG69GG66ege6egegeeB6eee6e6e6egegegBeBgBeBeBeBeBeBeBeBeBeegBeBeBeB6eeeee6eGGg8e999ttHCZZk2OAUtU2km0Djqp0tpkGUAv4k2hjjmmUkUqrcb:lvCy2kkvyyYcryUyLyU;mmrvZbVCvA:vmHvvVvvcAvvHyh;.v;jv y8VXCHYrhyv;yM.Cc;UZ v,:h,;Yv:  ;;;        Omv  H80qCqCqCqCqC@C0b@Cqb0CqCq
eDDD9D#296DbZOG9G689gOOO9#9CCcZDOGO#9Zb#OOB9#OeG6GgO9OOZeeG96ee6eee6egege6ege6Bge6ege6BeBgBeBeBeBeBeBeBeBeBeBeBeeeBeBeBeB6e6ege6e6eG666ZO#Ztj9CMZ9UkmcCNNZZpbbCtmp0wvrcpqjcUjptX2YNZvvwpkMkHryvvplyUcvvm;kcV;c#h0rc;NHHVrvy;AhYvm;M:,hvj; kBVXpv;AyyvypV:pA;tl.vvvV ;vl.  l;.        mH, ;6GqCqC0CqCqCqCqC0C@C0Cqb0C
D6Z99G#C#eG9OZ#O9BeeZO96Ge6OZpC0tq#6e#DOCq#eOO9eGOO9O99eGG969e6eGe6egeee6B6eeBee6BeB6eeBeBeBeBeBeBeBeBeBeBeBeBBBeBeBeBeBeeeeee6ege9eG66gOOZqk0qmU89Tj2ZcT0CmbCjC2c#hvANt0NckUZjCMrwOAl#Myvm0lvlvHlTk2LvH;HUvvpZccvYvyUh4AvT.McrAt;l..Uvm .cqlMrvvlvT.4k vkl;N;;M;Lv .;L; ;:;.        24 .CBDCqC0Cq0qCqCqCqbqb0C0C@b0
#0ZZOOeeDO6Oe99DG9OeeG6ZOGGOeO9#ZZ#Z9j099Oe@#OOCqOOq0pD9g9OZD#OO66eeBgB6egBeB6eeeee6e6BeBgBeBeBeBeBeBeBeBeBeBeBeBeBBBeBeBeB6ege6e6eG69B9OOOqC0OZpO8hwttZZtjpZqjC@mcmMVkt0N#j4U@OHkmZOyCA4lkBLvk;wVkMkTvAvvL;MZ#wH.vV;XmArXh.kMMvh:y;;y;A yCVUy;;:;4h,ry.MUm,U,vX,vv ,;V; .,v   .     0v,reOC#Cqb@CqC0CqCqCqC00qC0CqC
qbb#pD0CG8g9Og6GOGZBe6OO#ZGeOe9O@O@ZOOjZD6eBOOOOOg69Z9OGG69G9GZO9egB6egeeege6egegeeBeB6eeBeBeBeBeBeBeBeBeBeBeBeBeBeBeBeeeBeB6e6eGe969GO6g9ODZ9qCHk#eXHXc#j0ZCOmptthZHlHCjZjZmrjjjkjtDyvmNv;eqAHvcrMUhNvAVly;k9cAH,vv.ctyvhAvwlU;y;Mv.hXy.w0XjYUV,:UYvLy;lkU;c AA v,,vv;v   v         M;l0G0Zb0C0Cqb0CqC0CqCqC0CqCqC0
CC0C#@@jcjD969e6eGeGeee6OcjB8ge#OGBeeOGG6OG9GO9GeGG#9e6G66e6Be69ege6eeege6eeegegege6B6egBeBeBeBeBeBeBeBeBeBeBeBGBgBeegegegBee6eGeGG9GO6GGZOGO@ZO6tr96NHt#q@2Cj2cbjcbqyTjtqctCyUhZmjmDUlCUY;k9yl;4yyUkUAyVLr:ytCAc;;v,rUlvvc;llp;X;y: 4w.vvOXH;L:,,MvyUV;NUy;k.Cv;y,;;;;; ,,;         :VjG@Zb0C@b@CqbqCqC@bqC0C@CqC0b
#@OZ0CZOZ0jZZCCOZ9DOD69eB6qkb8B66Be99eee6e9GBB999e6O9B9G9G9G96eBeBeBge6eeBgegege6egeeB6eeBeBeBeBeBeBeBeBeBeBeBeBeeeBeB6eeB6eeege66G6OG99ZOOGOZC96ZUMB9NmH0C2pCcc#CtU0CLkU#jUpwXUHtXtCjlmwHylZHvvvrvUM4ylTy;;.tjAYv:y;vMrVM2:vyw;U;v :t; ;rBNAyV,,;TXY2VrVb;VX;k;yv:v:;v. ,:,  .      XHgZ0bCCqCqCqb0b0C0CqCqCqb0CqCq
OZqpCOGZOGe999GO#C#Z9GZZ86eq2cOBZG8G99e6eGegeeBOGGBOGee9egeGBGegeeBBe6BgeeBeBeBee6e6ege6BeBeBeBeBeBeBeBeBeBeBeBeeeBee6ege6egeGeG6G6969eD#@G9@Zqt#ZDrT9OmwCZcCckC0ttkqpkhpc2ccU4rU0UptcUkHVNvmjMvVLTkrvAvkyL;;pch;V;vvVCUvvU,lyX;k;; ;k.,Vk9lVvM.,vkyMNvywc:N;;ry;.;;,y; ..          ;t@ZDpqCqb@bqb0Cqb0CqCqbqC@b@bqC
j9e9m#qq0ZO9OeGO999OGeZCqOOBBcT6Gg6eee6eGeG6O66696g6OeGe6e6egeGe6e6e6egeeege6e6eeBeBeeeBeBeBeBeBeBeBeBeBeBeegB6e6egeeBeeeeee6eGe969GOGGO#O9GtCO@CD#ZYHBmltcjqjcO2ccp#HMmDmb@jcHMUGcckZCXhYXrr0H;rTXkU:VvyvkV,4jH;T;;;AHrv4y;vrMvU;v vw :ytZvVLl  vXYjT;AUy,C lyw,.;vvH  .,,         AO9@#qC0C0bqCqCqC@C@C@Cqb@C@C0bq
ZOB6ODCcbjcH2DG98geGGe8e9OjUC@9e8996B6gGe6eGgG6G6Ge669ege9e6eGe6eee6eeegeeeeB6e6BeBgegBeBeBeBeBeBeBeBeeeeeBeBeegBee6Beeee6e6696GGGG9GOGOOOG9OZBDOZZ9DwHOOZUUbtjtbbjkZjwkjcpttUjjXUCmU@ClyX2Tyqp:yAUyc;vvVv4k:vCmlUYvvANyvMy:,lLvT,L rX ;XbGAMy;  vXYjLvvy:rX yyh;.vvMU  :;.         Z6O00C0CqCqC0CqCqC0C0C0bqC0Cqb@C
jHjZO0b9ee0tm22OZZqGD9OeB86G2j9gGgO9G86GGgG6G6GgG6Gg6eGG6eGeGe6eeeeeee6e6BgegegeeeeBeBeBeBeBeBeBeBeBeeeBeBeBeB6eeeeeee6e6e6eGeG6G69G9g99DODG#OObZZj9OCH0jCHjtjtj@0kjt0mNU2HjjcUkcHUXmjO4vMmklteMvXUyXh:;;;vUvvHjrM;vvLjkLMy;;Vrvv.: tc vUb9UVv. .UmMcll;vvH  AXc..vVN;  :;         vB9CqCqb0C0C0b0bqC@Cqb0CqC0C0C0Cq
CD#cjm#O9e8BBeO#cjjtO@C@9O9G89B9OGB99eeG66eG6G6Gg9GGeggOGGgGe6e6egegegBeegegegeeBeBeBgBeBeBeBeBeBeege6egBeBeBee6eee6egeGegeGe6e9eg696GOO9#D9bZ6CGqmqqjOhXtCZbHb#bkcpkj#NHUkjqcjvOmbrCUbpNvlUHlChwkAvrc;:. ;;VrUpTv,VYlbUNVvvyvrXv   8m lNp8Mvv, .AUAUyvll;k  vmy ,v;Y  ;,          C8bqC@C@C0C@C0CqCqC0CqCqCqCqC0CqC
tG9#bqptHCDGO9B89#OGZCc22#06O6GeGOZOO69G9eGe9gG6Gg9g6e6g6gGe6ege6ege6ege6eeBeBeBeBeeeBeBeBeBeBeeeB6e6egeeeeBeBge6Bee6e6e6eGe6e6GGG9gG9O6DDOG0ODpbjCDHtHcO#ptHCCcN2ttjjctmmtbjctMcNwtcUmZpwvrpvAHvr;vAjVv; :vl;hHVv;wl;mUv;rv;v,rv   B; LrO@,vl. ,cvUkvvl.,A ;Amv,;;:,  :;          89qb0C0C@C@Cqb0CqbqC@C@C0bCC@C@C@
pCbZBGOZ0ZCH0ZDe8Z6BeO9ODG9D#Z9ZeOO96O6GeGeg69e9gG6969eGe6eGeGege6Beege6egBeBeBeegBeeeBeBeBeBeBeBeege6eeBeBeBee6e6e6ege6eeg6e66G69G9gOOGO@GOqC6@D00CjOZbOq#jj#CXHjjc0mtmMHtcttTX4CLXmc4bOqv;@trtHyyyXLv;l .Y4:qhlvvvM;XkVvH, ;:yv  :8  vpBM:M;  vkymU,lA:vv yVV;;.:l  ; ;.        VBD0qCqC0C0bqC0CqC0CqC0CqC0CqCqC0b
b02bcjO8GBZZG0cjt##9ZOe8OGGGZO6ZZOOeGOD999O99g96969g6eGeGgGe6egegeeege6egeeBgBeeeeeBeBeBeBeBeBeeeB6B6eeeeeeeeege6egB6e6e6ege969g9GGGO6DGOD9OZeZ0CUjgbOjXmpccjZjmtmcttcUtk4LbctcbVZHkNUvLZby;rkl0pUkyAkV,y, :rvjjwvl;v;c#.vc ,UvM:  vB .ht0U;;;  tw;y;,VH;m,;yvvv  YM ,.;,         Z800C@C0C0C@CqC@bqC0bqCqC@CqC0Cqb0
@tHtqbCpq0De8eeZCCccZ98eZZ6geZg9GOGeBGGO69G9gG69696GeGeGe6eGe6e6eee6eeegB6eeBeegeeBeBeBeBeBeeeBeeeegege6eGege6B6BeeeeGe6eGeG69G9G6BOO9#ZOO6O#@tpqtGZkjHH@Z2ktcjCjcbtjkc0ttXUCUtcUXjHUrHTmHkrLHvHhLwvN4Uy:; .vvbtm;Y:r;02 lw vyvA.  r8 :yckj.;;  bl,A;;yV;r ;v;y, .0Y   v:         892bqC0CqC0CqC@CqCqC0b0C0b@C0b0Cqb
0m4pbmHqqqbqCOgG9GGgO9969gGge9GgGGO99gGeGgGe9g96Gg9gG69egeGe6eeegegeeeeeeeeeeBgBeeeBeBeBeBeBge6egeeegB6BgB6eeeeeGgGB6G9egG9gGG96OgeOOgD#DGDG202O0##ttDpZbCHc#HkZwt0cmtppktHkct22mUkUtcmkyjcphtVrL4NlhUVk . .L;Uky;vrAvck Av.;VYU , @2  kOHm... :@YVU.;;mVr Nvv;  ,X.  ,l         :8OtqC@C000C@CqCqC@b0CqCqC0bqb0C@Cq
O@j@GZC2CD9Cb@ObGOe99OODGG6GgG6G6GGOG9g9GGG96969gG69e9g9e6e6eGege6egegB6e6BgBeBeeeBeBeBeBeBeegegeeegB6egege6ege6e6eee96ge96GgGG9gGeOeO#OBGqZeOjZZcDbOOC#mM#O0Njp4HcMwHpk4kOCUH@jjrmkjjmAmcUtUkyLwwvv2Uv:v ::v;#hT;;lT;Hl l; ;Vrc . 9;  AeH0    vj,lv .vXyv m.v;  rv   v:         l8C0bqCqC@C0C0b@CqC@b@C0C0bqCqC0CqC
O69GDZ#9OOD69@CD@9OOO9O9O69GGeGGOG9G9G96GGG69g9GG6G696G66gGe6ege6ege6e6eeeeBgBeBeeeBeBeBeBeB6eeBgegeeeeegBeeeegeGg6e6G9eeG969G96e69GeZCgOC2GOtcebmD@q2kZNw9OHZcNhtctpqjcmq#cHCCbpr4bhm2krgHqAjXhHL;VtTv;Z;:l;;OrY:;4Uvrv N.,vMkL ,.q v Hqrj   .Uk vv,:v;4  V,r  ;y . .v          @Bb0C20C0C@C@CqCqCqCqC@CqCqb0C0bqC@
2t@0#Cq0DCqZ6OZ9OD#Z6OOqZqOOeGG9G9gG69GG69GGg96G696G6GgGe6eGgGegegegegeeegeeBeBgBeBeBeBeBeBeegBgeeegeee6eeeeeee6egege96eB9G96G6gGGBD9OO9Z@GG#COOmCetccmbcbOHtkHHTjCcCUUj#cjtCjkkOmACMLHUU9jXU#cvUAvycTv:bk,vv 0mM::Nr;tv y;vyMk  vvc,r wvmg   ;tk y,;y;Yp lvv..:w: . ,,         ,BO0qjCb0C0CqC@CqCqCqCqCqC0C0CqbqCqC
Gbktcmp9Oq@G#9OGOOD9DZ@999Z99gG69GOG9G969G96Gg969GG6G696Gg6eGe6e6eeegegegeeeeBeBeBeBeBeBeBeBeegeeB6egeeege6egege6e6eg69696OGGe66OeGOZDG99B6GD90#bO02j#jOpp24HHkjH2cwrkcDjUHjpCmCZOXkUrLrMpkkAqt,Myllth:vAm: v:ObUV;V;,bv.v,,vAh  lHcvm V.UG  vvtl Y,;r;U; yv,, Ar   .,          h8qqb0CqCqCCCq0#C@b0CqCqC@bqC0C0CqC@
00OG92jpZp@99O9D9OOO9DOOe999GOG9e96OG9G9G9G9g9696GeG6G69696Gg6ege6eee6egeee6BeBeBeBeBeBeBgB6egegeee6e6e6ege6e6e6egegB6GOGGG9G6eGgGG9ODeDG6Z##ZZDDCC@Cjtm#hUUqCHHcU4httcjccpcb2CO2ZCXHUTwlyVXXL#vmrvvw4vvwVy vM8hUy,v;;C;:l, vMw ;MevNc y.T8  vvA;:V ;vl;.:;.;,,r    .           9Gqbqb0bqb0CqCqC@00b@CqCqbqCqCqC0C0C
mHcp0OOGqDDD@OOg6BGGGeG9O9O69GOO9e9G9G9G9G9G96Gg9G9g9gG6969g6e6e6eeBeB6eeBeeeBeBeBeBeBeBeeee6egeeegeeegeeB6e6e6egegeeeG6969gGg9gg69GOG9OO6@0DO0bbZkptbbt#cUbCHmUNtpjkmUccC2pCbmpcD@4htTTLm;rMrt;U0hv;hw,vXUv ;eyMM.v:lp::Y  vMy;h2B AU,l bB  ;vv,;; ,Vv;.:.:, v  , .           ;BGpqCCC0C0bCCqC@CqC0C0CqbqbqCqCqC0bq
O#Z0#bq0OODqODOZ9OgGOG6DDO9O999DZZOOGGG969G96969g9696G6G6G6GeGe6e6e6eee6e6eGBeBeBeBeBeBeeeBgeee6B6egege6egegegege6e6ee6GGeeG6O6GeG6OGG6@G6DCDCZ2Z#jjqObUbttUHttXUUcHHHXkmtq2ZCctO@qwXjckytvyA40;k#m;;;t,:rHp,;BvyL,v;Yb;v;  Yrwt4p9 TH;,.g8  :Uy;., vUvv..;:,:v Y .            v8C@b00qb0C0C0bqC00Cb0bCC0CqCqC0C0Cqb
960#COC2CZ#OO6GG99DDO9OOO996GG9GO9OG9G9G969GG6GGG69g9gG6GeGe6ege6ege6e6eeBeBeBeBeBeBeBeBeeee6egBgBge6ege6eee6B6egege6eG6GG96OG9g9G9GO6DD9e@@9OZCbCD0j@wt0tmktZNr4UjtHmXUk#bkUjpODCjmUtkkYkMyrvt.VmYr;:0v.mLrAwC:vUvv;Vkvv;  vMlkAG4 49; :8B  ;wh;,  ;v;; ,:, ;  v              keqC0p0CCCqbqb0b0CCbqb0b0b0b0CqbqC0C0
GOO9OOOOGOGG6G69eGg9G99OODG9GGgeG9G9G9G9G969e9696969gGg9G9gG66egege6eeegeeBeBeBeBeBeBeBeBeBgegBgege6eeegegegeee6egegeg6Gg9eGG96G69GOG9D@99qC@CO@O#Zcc0b0CwcCcUAUqHHtmkXkCjUUHt0kHp##Z2khm4ZXtv2 U@Yy;;0;:HMyTwmvVry;;rr,L.  Yv;VHg: jO. ;Ge   ;v;  ;:vv . , ;..;               t6C0b0b0bqb0CqbqC0bqC0bCbqbqb0bqb0b0C
Oq#O0ODG9G99Ge9GOGGe9G9G9GZODOOG9696OGOG9G9GG6969696GgG6GGG6GGge6eee6egeeBeBeBeBeBeBeBeBeBee6eeeeeeegege6e6egeGe96Gg6e96G69G969696OG99DZOGCqODZCqZ#qpqO2UpZ#HmhHtXVc4hm@HwMHHjjUtObbqZcbUmCwmvt;kpvv:lc,.yrrLhmhl;yV.r;:m. vV;;vUb ,m9: :99    v:  l;y;.  .;; v,     .        ,99Cb0bCCqCqCCC0b0b0C0bCbqbCCqb0Cqb0C0
ZZOZ0ZOO0j0Z9Gg9OZOO9OGOG969gG69G9G9G9G9G9G9GGG969g969G9G9GG69e6e6e6e6B6BeBeBeBeBeBeBeBeBgB6e6Bee6egegeeeeege6eG69e6eG6Gg9GGG96G69699OgOg9OOGO9Cqtt0#t9jX2ckbtcLpjjpUUcXUUUTUHmUHcHkD@pHyUmAp;TlkbTYvcq,;vNMA4tYl;lv.V;yH  vv:;,LX vLG; ;Bb   ;v; r;vv    .. .                ve920b0p0b0C0bCC0b0bqCqbCC0C0bqCqbCC0C
@9OGOGO9O9b#0#96969G9G9696G6G69G9GOG9G9GOG9696969G9696G696GgG6geGeeege6egegeeBeBeBeBeBeBeBeeeB6eeeee6egege6eGege9G6e6gG696ggOGO6GGOG9G6OOGqOZO@@bCCCpCDjhCjcjmHkUmHHcmAHkHrLr4NhAtmkmCCcHmAUmYAhAthvvAcvlvLAttMl;lyv,y,VL  ;v;, VV.M;Cv v8A   v; ;l;;  . .  ,:                w8@qCCbqb0C0CqCqCCCqb0Cqb0bqb0C0CCCCbq
eOGO9OODO6BBGq9G6Ge969G96G69G9GOG9G9GOG9G9g969G96G6969G96G6GgGegeGegege6e6B6BeeeBeBeBeBeBgB6e6ege6e6eee6e6e6e6B6g9eggG6G6Ge9GO9969GGG9696OOOD#ODqp6CZZCm2OZttwU4XkkXjkUjk4hyAwUUNmtktjtC#kU0jrAhvycvrvjlrvlvmMh;v;Lv;r.VA .:;v: 4:vv;wy ;eV   v. v;y  .  . v,.      .         2e020bCb0CqCqbqbqb0b0Cqbqb0CqCqb0b0CCC
96ZOO9O6OO9OO9D9OG9G9696Ge969G9GGGO69GO69G9G969696Gg9gGg9G9gGG6e6eeeGe6e6eeeeBeBeBeBeBeBeBeegBge6egege6e6ege6eGe6e6eGeGgGgGeG69GG6969G9e99O9OO@#ZO0C2Dj0p0kUjmHmkcHH4kTHAhUrAkNkAkHccmctUcmUkHTcLX0Arlmrvyyvwhw;;vwv;M yv , .v. v.vvvvv ;B; , ;;;vH: .   ..         ,        ;G9bCC0CCC0b0bqCqb0bCC0b0b0b0Cqb0bqb0b0
OZ9DOZZ#OZgZGOOD9OGGGO6Ge9G9G9G9G9G9G99OG969G9G96969696GG9GGgGe6e6ege6ege6egBeBeBeBeBeBeBeB6eeegBeeeeeeeeeeGe6g6e6e9e66G69GGeeG966gG6GGg6OeOODOOGOqO##Oqjjpj@HUXkmcNNHAAmkkyXkTAAwHHcAMmt4kNjkTUXhHVvymhvyr;wvrVVYV;;v,4. ; ..   :v;r:;;;0. v ;;,v; ,    .          .        Y800bCbC20CCCCCCb0b0C0bqC0b02CCq2CbCCqC
9GO69GO9OGGG9GO6GG9G9GGG9G9G969GOGOG969G9GO6G696969GGg96Gg96G66eG6Ge6egegBeeeBeBeBeBeBeBeBee6eeeeege6e6e6e6eGeGg6e6e969g9G9e9G9G9g6gG69eD9GZZG#O6O#OCj0Zcjbj4kkmUcmmmcr4bch4ytUhTUUUwhNqkVXpttAUjHVvVwUUvyVvv;mN: ;v.l;v ,;      ; ;V:;v,T .w ,:   v.    :                   wB#20b0C0b0b0C0b0bqCqbCb0CCCqb0C0b0b0b0
eGGO9O9OG9G9996969G9G9G96969g9G9GO69G9G969G969G9G9eG6Gg9696G69eGg669e6e6e6BgBeBeBeBeBeBeBeB6e6egeee6e6egege6eGe6eGegeG6G6969G96969OOOZ9OOOeZDDDDOpjjqtZCmmCjcCtkmHHhwUpckwUkMNhUmrrXHhA4X4qbkCtlAtArkrw4vvYyy;yv,;V:;l;;,   .   ,. :V:v;:V .A ;,   ,    ;                    r8qCbCb0b0b02020bqbqC0b0b0C0bCbCCCCCCCC
#e66O9969G9GO69G9G9G969G9G96969G9G9GOGO69G9G9696969G9G9G9G9GGgGeGeGe6egegeeeeBeBeBeBeBeBeeee6e6e6e6egegege6egeGg6eGe6eGgGG9G9G9GO9#ZZOOD#9O9ZqZG0Cj#OqD@UpO0UtptHcHkwcmkAhwkMNAhryrkUAyNmjkT4jmXXmrUwvrkv:lUAvrYv;;;l; ;  ..       ;v,;,:v .l;;        ;                     wB@C0Cq20C0CqCqb0C0b0CC2CbC2C2CC0b0C02q
ZDe9OO6GG9g9GGG9G969GG69696Gg9G9G96969G96969G96969696G69gG69gG69G969g6e6e6egBeBeBeBeBeBeeee6e6ege6e6eee6e6egegg6e6eGeGg6gG6969GOGOOD9ee0DDZ#G@DO#q#20jq2jp#mUtCmNwmHkcUwHwXyrXTlyyMrNhrrNhUkALUHwrAAv:AUAvXUcry;Y:vYl; :;;;     :  ,;., v;  vY  ;     ;                      M800b0Cqb0Cq2CCqbqb0b0Cqbqb0bqbCC0CC2C2
O6G69G9G99OGOGG6969G96GeGG9GGe9g9696O69G969G9G9G969G9G96G696969GGG6egeGe6eGe6BeBeBeBeBeeeBeBeeeegBge6eee6egeeB6e6e6eGeGeG696Gg9GG999Zg#qDe9O#ZqGD@@tCZtpDjHCttmtXkHHXUVAXXwyAUryArrAkkrMkbmkvycUvrXyvvVTrMlLhMvVv;v;;;:V.      ,,  .;: .v.  vv ,    .                        Xe@C0CC2CCCCq2CCqC0bCC0C0C0b0b0b0b02CC0
#O9OG9g9GOG9GO69G969gGgGg9G9696G6969G96969GOG9G96G69G969G9gG6G696Gg6eGe6ege6eeBeBeBeeeBeBeBeegegeee6eeeeegeeeee6e6e6eGeGGG6GgG6OOOGO#OZ9BG9@DZq9DbqZ9bt0Zt0qCcmHkjNXUyAUMryANYvrLVrymXAwkXUUVUj4ThUylVryylYr4Avvv ;;.;l,       .,   ;. .:  .;;      ,            .           NBC0C0b0bCb02Cb0bqC0b0b02Cb0C0b0b0b020C
D9OO9696G696969G9G96Gg969gG69gGg9G9g9G9G9G9G9GGG969G96G6GgG6GGG69ege6eGege6egBeBeBeBeBeBgBeegB6eeegeee6e6egegeGe6e6eGe6e96GG6gG9OG6eOeGeOZ#GDO0GqCZDcj#Gmc2bkcjHcjywXrwXLyArvvVMVVVVAVykwkchykXwwrvvvrTyVVLAXv,yv.Tv.Vv  ..    ..   ,      ,.;                   y           Ue020pCp0bCpC2CC0bCb0bC20bCb0CCCqC020CC
99G96OG969GG6GG96969gG6969gG6969GG6GgGG969GGG969G969G9G9g9gG6GeG6ge6eGegeeegeeeeBBBeBeeeBgBeeee6eee6e6e6egegeGe6egeeg6gGeG6GgG69ODe69G9ZOOO@OZOO@tC@bbObcj@cHjpmmcXHcXXLTVrrYvryvvyYhyVrt4lvV4UvvyVvlMNlyhA,hAv;v;r;;v;;. .    ,:   ,   . , .                    v,          k8pCpbpb2C2b2bj0bCb0C0CqbC2qb0C020b0bCb
OO9GD9DGO6GeGGGgGeG69696G6GG969G96G6Gg96GgGeG6Gg96G6GeGg969G9g96G6Gege6e6e6eeegBeBeBeBeBeBgeee6egBge6ege6egeGe6egegB6eGg6eGg96DOG6OODC#G9O#O#O#qjbZDZOCb0qp0CpktkhUHUTrMrrlvvvlyvvVyhAlTcMvUykwlvLlYyULrrMr:kv:.vy,;v,,;       ;       :   .                     ;,          HBCpC20bCb02CbC2Cp0pC2C2CbCC0C0C020C0bC
OGe9G96GeG6O696GgG6G6Gg9696GG96969g9g96G6G69gGG96GGOG9G9G9G96GG9GGG9e6eGegegegBgBeBeBeeeBeBee6e6e6ege6BeBgeee6e6e6eeeGg6gGGOG9696@##OZeZ2COD@qZCZ9#pCjcbOqqbjkkUUkcMNhwAAy;vAlvvvvVlrYyMAyYrlyy;vvVyAhrrrAA Avvvvv;vV:., ,     ;       .  ,                      :v.         yeqC2bjCjbpb2C2C2CpCbCpCbCbCp02Cb0b0bCb
6e99D99eG6999g9gG69gG6GGG69g96GgG696GG96G6GgGe9g96OGOGO9OGOG96GGG69gGe6eGe6eee6BeBeeeBeBeBeBgeeege6e6B6eeeee6eGe6e6e6BGe9G9GOGGeGZZ6#96qjO9ObOG9Cbj@b#OOtCC0jjmtcjtUwUwAyrvrNv.vvv;yVvryvMv;;VYVyAVAVAyLyMv 4Tvl.vr;:;v;      :.       .                         ;v          ;GOpCpCpbpbpC2CbC2C2C2C2CpCjCpCj2C0b0b0
CZOZOOO9OOg6eG6969G9G96Gg9g96GG9696G6G6Gg969e9696GG9GG6999G9G9G96GGGe6eGeeBge6Bee6eeeeBeeeBeege6eee6egeeB6e6eGeeeGeGegeG6969GOGG9tZ90#9Z9G9#ZZObpbOj06OctC#pjcjCpUkXUUUrrrUvY;;lvvvvlvLlMLVvvVyYrVYALyrvLrv.Uv;v;Yv.;;;       ;        ,                          v.         .OOCpCpCjbjbpbp0bCpCb0bCpC2b2b2C2qb0b0C
0ZO9OGOO9gGeGeG6G6969G9gGeGgGe9e9gG6G696G69gG6969G9g9GOOOG9GOG969G9G9e6eGegegeeB6e6e6eeBeegeeBeBeBeBeeeeGeGgGg6egegeG6Ge9e9GG6O9bC9G#eGGO9DO0OZ0q6O0CDUXCOttctjjkkkmHHNhrXylvvVv.;vvvyAr;llvY;vrvLyUhYlYAX:vXlvv;vv. v.      ;:       ..                          ;:         .06bCpCpCpCpCpCpCjC2CjC2CjC2CpC2b2bpCpC
eZgG9O69696GG96G6G69G969gGgGgGGG696G6G696GeGgG6969gGG9G999GOOOOOG9696G69G96G69eG66g6e6eGeee6egegeeege6e6ege6ege6gGe6ege66G6G69ODOGe9O@Z@DZOO2G9j0jtCCbctZOtcpcHXkUHHUhhAAryYLvl;,;vvvlvyyyvvv;vlYyUXMvlYhv,vNlv, v;.r,      ,y       ;.                      :;   ;,          b9C2C2CpCpC2C2CpbjCpCpCpCpC2C2C2CpCbCb
eeG9O9969GOG9G9696969g96Gege6ege96G6G696Ge6e6eG6Ge6e9gGGO99GO6O99G9G9G9GOG9G969gGGGg9G969gGe6e6g9g9gGege6eGeee969e6egB6e9eG6O9OOO9G9C0O9ZDZ#0GCmCODe00j209mmHmmUccMwNUNUNTYMyvvv;v;;;vYyvv;;;vvlYTAAvvlLv;,vLl;;;L.vv..     Yl      .y.                       ;, .;v          cGbbjbpbpCpbpC2CpbpCjCpCjbjbpbpC2C2CbC
9D#q996OG9GOGOg96G6GgGgG6Gege6e6g9G9g96GeG66e6e66GgGG969G9GOG99O99G9GO69G9OOGO6O696969GO6G66e9696GG9gGeGGGG969G96GegG96GeG9O9O96eDg90e9OOZOZZ#ZpCOZ@pqOC20tcmXmU4hMHhrwrALrMyvvvv;;;vvv;;l;;vvvvVrvvYvyvvvvvY;;l;:vv::     ;L       . ,                         V;;r .        tGCjCpbjCpCpCpbpb2C2CpCjC2CpCpCjCbCbC2
O9q##OO69G9GO6GgG6Gg969gGe9gGgG69g6eGeGeGe6eGe969g9696969GOGOG9GG696OOO9OG99O9OOOGOOOOD9O9OGO99G9G99DODOOGG6Ge6e9GGeDDD69#Z9OZZeO0GOHOGeeZOg0q#bCO2qp00tcbttkUhtVUkALhU4yXArvlvyvv;v;;;vYr;vvv;vvYvlvl;vvl.vv.vv:;vv;.     ;v        :                            ,Nlv..      UG2bjCpbjb2CpC2CjCpbpb2bpCpb2b2bpb2CpC
9OGOGO9O69G9G96969GG69696Gg96Ge9G9e6eggGe6egege6G969G9GOG9G96OGDOOOOOZODODGO9O9O9O9O9ODZOZOO9OGO996OOOOO9O9966699O99GOGGGDeGOZGZCOeOq0BeeOOOO@G@O9O#0C#tCbjcmk2XyHUyUTLyrArllyyv;vv;;;vvVv;;;:;vyVvvlvvl;v:;v:v,;vvvv      ;;        l                             vL  ;      mGCpbjCpbj2pbpC2CpC2b2CpbpbpbpC2CpCjC2
O9969GO9OGO69GG6G6G6GgG69gG6G6966e6eGe6eGe6eGe6e6GgeGe9G9GO6969GO9OOO9O9ZOOOZOZOZOZOOOZOZDZOD9O9OO9eGG96O9OOZOOO#ZZ9OGOZ@OO9OOC0O8eD0OGqOeOep2#C20jCjCCtXkUH4UXyyULArA;vkNvVVyvVvv;;;vvlYv;;;;;vvlvvvvvY;v,,;.: vv;v.      V;       :U                             ,,:.       mBbbjbpbtpjbpb2bpbpCjCpCpbpCpCpC2b2b2C
699O9O999OGOG9696969G96Gg9gG6G6Gg6egege6egBeeGe6e6e6e6eGeee6G9G96G69G99OOOOZOZZ@OZDZOOGO9OODODOZOOOZDZDZOO9D9OO#OZOZ9DO@#q0OeqCb9ZZCZeUr66#tmc#C@@ttCjtMwNUXUUkrrwhyAryTkvvVyvvyY;;;vvvvl;;;. :;vvY;;vyvvv, ;:,.v::v      ;v      . b;                              ,Y.       kG0jbjbp2jCjbjCpCpb2bjC2C2CjC2CjbpbpCp
OGOGO69GO99G9G9G96969GG6Gg9GGe6e6e6e6ege6e6ege6e6egegege6egeGGOOO9D9OGOOZOZOO9ZD#OZDZDD9OODOOOZDZOZOZDZD#DZZ#D@O@Zq####GO2p9D@#0Cjm#DqwHOpjbmjtpccc22tHtmHXkkmkNMk4whTrAyYYLvvvlv;;vv;;vvv;;,,:;;vvv;vYv:v. ,: ;.;.,     .;v      . j                               .v        heppj2jbpCjbjCpCpCjCjC2bpbpCpCpC2CpCjC
OD9O9O9O9D9OGOG9696G6G6GgGeG6GgGe6egegeee6Bee6eeegeeeeeegGeeegBegDDqZ@Z#ODOZDZOZO@Z#ZqOZDZZqD#ZqZZZqDZOZOZD##CZq@bbpjj#C0pjb0bpmtkHcjcbktcqbmcjctHtjpHcjjtjcccmHc4UkUAALAVyYv;v;vv;vv;v;vvv:;..:;;vllvVv:.. .;, .;::     :v,     . vL                               ,v        jObtbjpjbjCjbjCpCjCpC2Cpb2C2b2bjCjbpbp
OOO9O9O9OGO9OG9G9G9G9G96Gg96Ge6eGe6egeeeeBeBeeeBeBeBeBeB6e6egBGeeBe89GDO@ZCC2qCD#@C#0@C#CC20jb2CjjcjtptbC0b02qb02Cp2ccjCjtCjtbmtcjmtcptmmttcmctjmcmjjckHUUUcmcUkXkkUhTrNAryyllvvvvvv;vv;;vv;  ,;:;vlvvv;... .:: . ;:     Yv        C.                               ;v       :OOjbpbjbpCjbpCjb2bpb2CpbjbpCpC2CpCjbjC
OO9OGOGO9O9OG9GO9OG96GGG69GG6GG6e6eee6e6egBeBGBgeeBeBeBeBee6eeeGeeegBeeeeeBegZZbCCCpCp2pbj2jtctctmtctHmkH4HUkXHkmmjjbctjpUtUUmj2bcjtttcmckmtttcmtmjckwXUkmckkHpUkmmNNAyywyAVyyllVvvvv;vvvl;.,;;:;;;v;;v;,,;.,., .:v     ;Y;       ,T                                v;       ;BZbj2pbpbpbjbp2jbjCpCpCpbjC2CpbpC2C2bp
ZDDOO9O9OOO9OGOG9G9G9696G69g96Gg6egeeB6eeeeBGeeBeBeBeBeBeBeBeBee6egBeeGG9G96gBG6O9OO#Z0DZD#q#Z@ZOG6Be8eeGGOO@#CbttcjmcHcHcUcktq2pCjjmmkCCjktj0kHjjcckHkHwmckhtHkHcUHkmAhUTArAkrvvvvv;;;v;;;v;:,;;;;;vvv:.,:.     v:     vv      Y vl                                ;;       reqjbj2jbjpjbjbpCpCp0pC2b2CpCpCjC2CjC2C
O#OZOD9O9D9O9O9OG9G9G969G9g9e96Ge6e6e6e6e6BeegeeBeBeBeBeBeBeeeBBBeBeeeeeBeB6GO6eBgeG6OOOGG69eeBe8BBe8B8B8B8B8B888B8BeG9qCppj2ppkHctjqC#pCq#tC2bctcjtcmHUUHm4UUkNUXUUHkk4UrrU4lyyVvv;::,.,,;;,,;::;;:v;;, ..     :;     .y;     v: U,                                ,;       cBtbj2jpj2jbj2j2jbpC2CpCbbpb2Cjb2CjCpbp
ZODOO9DOOOO9O9O9O9OG9G9G96GGGeGG6e6eGegeeeeBeBeBeBeBeBeBBBeBeBeBeB6eeBBBeeeeGegBeBeBeBeBeBe8e888e8BBBBe8eBeBe8B8B8B8BBgeGGGD#ZpjttHmHcttt#@@0@OGZDp22ptcmcHHUkkcUUUUUUU4AHNhyvvyLlly;:;;:;;v;;. ,;,.:;;;,.     ..,     L;      . Tv                                 ::       D92tbjbjbjbj2j2j2pbpbjbj2jbpCpbjCpbpb2b
OZD#OOOOOOOOOOGO99G9G969g9GGeGe9eGeGeGe6B6Bee6BeBeBeBeBeBeBeBeBee6eG99G99ZOOGO69e6egBBBge9698B8BBBeeBB8B888B8B8B8B8B88888B8ee6OOGZ0jbptmtmcktHj2bCD#@2p2bjjHcHHmH4mUkAhArwhMHULvylyl;vv;;;;;;;;:;;. ;;.,:        .    ;v        ;r                                  ;       ,9DjCjbj2j2pbjbjbjbj2pbpbj2jCjCpCpC2CjCp
ZD#DZDZO#OD9O9OGOG9G969696G69g6gGegegeGe6e6eeBeBgeeBeBeBeBeBeBeBeBeB6BeBGGG69G9GGgGege6e6e99O9OGOG6B9eBg9e6ee8B8B8BBB8B888B888eBOOOOO9OD@qCqCbHUTXHp#GOOqZ0CpCctmcHHHkUkhLlryXHNVVAy,;vv;v;;vl;..;..::.v. .      .    ;         y;     :                            ..      YebCjbjbjbjbjpjbj2pbjbjbjbjbpCpC2C2bpCpb
ODOZDZDZOZODOO9OGO9O6O696G6G69eGege6eGe6e6e6BeBeBeBeBeBeBeBeBeBeBeegBBeg6969egBeegBeBBe6Be8BeeeOZp@9O2D#ZCbtCbGDgBBgGZO@#CC2bccUHHpbCb0bq0Z0qCZ0jmHhXtGB8OZqZC02CjptjHkHcHUrvvUHAyAl;vl;;vv;v;::;:;  .;;  ,     ..    ,         y;    .,                  .         ,:      Z9p2jjbjbjbj2jbj2jbpbj2jbj2jbjCpCjbpbpbp
qZqO#OZDZOZODOO9O9OO96OG9g9g966g6gGe6e6e6e6eeeeBgBeBeBeBeBeBeBeBeeeeG69GO6gB6ege6eeBeBeeeBeBB8BeGGH0B8O999O#2CcUUrvUUUwwrLVyyLyrlYvlvvvyNbCCmkUmHtHjmUrM08BBZZ0qbbjptccjcmMTyAhAhMywAyv;:vvlv;vv,.:. ,.;:,       ,   ..        :V;    ,,                            vv     9GCtbpbjpjbjpp2jbjbjppbjbjbpbpb2bjCpCpCpb
@0ZqDZOZDZOD9O9O9OOOG9G9G9g9g9eGg9eGe6ege6e6BeBeBeBeBeBeBeBeBeeeBeBee6gGeB8eBeeee6Bee6BeeeBeBgBe8BGUjBe66OeGGDCmU; ;MrXHjZeeBe8BB6GZ#Cpc4vvYkUwyLyLVrATlvVbe8B6DZC0CptjjcHkU4hNrhArrNUwYv;;;v;;;;;;:. ,;::,     ..   ,      .  vh;    .                             ;.   ,@g@pbpbp2jbpbjbjbjbj2jbj2jbjbjbjbjCjb2Cpb2
@Z@Z#D@OZOZOO9O9O9O9O99G969GG6G6Gg6eGeeegeGeee6egeeBeBeBeBeBeBeeeBeB6eGee8ge6ggege6e6eeBeBeBeBeBgBBeHOe@OGOO#OCjtUvVkccjU0OOZ99ee8B888B8U   vUUmckh4lvvvyVvYkBBGODC0j2tjcjctmHUkNwyyrUHNVlv;:vvVv;,:  :;;v;,        ..     ;  ,k4,                                 :,   ;B9Cjbj2jpjpjbjpjpjbjbjpjbjbjbjbjbjpjbpbpb2C
Z0ZqZZDZDZOZODODOO9O9OGOG969696G6G69eGe6eGeeBgegegBeeeBeBeBeBeBeBeeee6ege6eO6Gee9O96egeeBeBeBeBegOGBBH9OjqGO@C0224VVt@CCtpOZOO66e6bYYHD8k  VhrhcZOOZcAwyVV4VvhGGe9O#qbCjtcjctmmHkXhyrryyNMAl;vlYV;;,.,;.;;v. .      :     ;   44M     .                           :;   tBZqj2jpp2jpjbjbjbjbj2jbjbjbjpp2j2j2jbjbjCjCj
qZ@ZqZ#D#ZZODOOOD9OOO9O9OGOG969GG69GG6Gg6egeeegB6egeeeeBeBeBeBeBeeeB6eeeGeGe6e6eG66g6e6BeBeBGBBB9OGeegOgqbbGO@tbtcwrACpC0DOOOGGe6Zy.:HeO:;XXAMXkkZBB0mkhkAvvy0ZDZGG6OO@0jjcttjmtHmXwrArXTyvvVy;vrMV;.:..vVv: .   , .:     ,  ycHy                                .;. v680Cjpjbjbpbjpj2jpjpjpjpjbjbjppbjbjbj2jbp2pb2C
@0#0#q##O#OZDZOOODOO9O9OGOG9G9G9696GgGg9e6eeBeB6e6e6eeBeBeBeBeBeBeeee6BgeGeeBge6ege6e6eGe6eeg6BeeZZe888#BG@09eqtcUHMAkC@#qGO6G6G6OHym9m;V4wrrhmNk#88BOCHHy;;kC9DD0DO69O#qbbtjctctHcU4wkhrvvlrl:;yrXl;:;;vvlv;;; ,. :; .    Y,UcLv                                ..;DBGbbj2jbjbjbjbppjbjbj2jbjbpbjbjbjbjbj2jbjbjCpC2
CqqZ@Z@DqZqZq#ZOD9O9DOO9OO969G9GG696G6gege6e6eGege6egeeBeBeBeBeBeBeeeegeeBgegeee6gee6ege6egee6G8BGjege8e98BOpCG8OcMXLyHZDeG6gBB8B8GBZv;yyLVyrwr4C8eBB89GjXvy2ODeeeOGGGO9ZZb0ptmckmmckchhrLYlVv;,;yvv;;vvVVvv;,:.. .v,:    ;9rXhy.    ...                       ;Nq0ZZ@jbjbjpjpj2jbj2jpj2jbj2jpj2j2jbjpjbjbjpjbpCpbjC
qCq0Z0#qZ@Z#ZqDZOD9O9D9O9O9OGO69696GGGg9eGe6eGeGe6e6ege6BeBeBeBeBeBgBeege6egegeeG9GG6GeeBgBeBGeeBGC9Bg8gZ@B8eCcpeB8@trYMtHOGGe8BBqHv:;AylvlyMVwpeBg6BB8B8eqk2DGGe6eggeB69ZCp022tjpjc2kkUUMAyrVh;vv. .:vyYvv;;;;.. ,::     @O;lUv,   , :                    . ;rZB@@0Cp2tbjpjbjbjppbjpjbpbjbj2j2jbj2jbj2jpjbj2pCpbpCp
C#bqC#0@qZq##DZOZOZOZODOO9O9OGOG9g9g9gGGGGGe6e6eGe6e6eeeeBeBeBeBeBee6eeegege6BgB6e6e6e6e6eGeBeG8BBDgBB6e9Z68e8eZp0G8B8eOpwhMVrY;, .;vVvvvyMAAHD8B86eGegBB8BBC9GO#9G69eeBgeC0p0DgO9b0DHkckUXXr4hVV;   :;vv;;v;:::..,,     eBv LHv,  .;,,                  ,,,:b860Ct2pbj2p2jbjbj2p2j2jbjbjbjbj2jbjpj2j2j2pbjbj2jbjb2b
@CqC@0#0ZqZqZ@D#OZOOOZ9O9O9O9OG9G9G9696G6Ge66Ge6e6eeeGe6BeBeBeBeBeBeege6eeBeeGe6g9G9eg69GGBe8G6e869OBBe68#OB9O8889Oqb68B8B8Be99@Z9G#0jtcjb@#8B8B8eeO6GODjceB8eeGeeeD99G9ee8e6Z0j@q0qGZjtjmUwmUXhLv;.;vv;;;v;:.:...,     UBZ  Nmv;  .;:;.... .         ..,,,,.;qbCC2j2j2jpj2pbj2jbjbpbjbppjbpbppjbjbjbjbjbjbjbjCjbjCp
Cq0qbqC#@Z@ZqD@OZOO9ODD9O9OO9996O99G9G9696GeO69eGeee6e6e6eeBeBeBeBeB6e6e6e6eGGGeO9GeeBZOGeeege6BeeGD9BG9ee#eeOGOq9eqXhXtCZOeB8B8B8B88888B8B8B8eBeG9OZCmbCDGee6eBe69GO9O99GGBee9OZ#2bpZjjUh4pcmUkyl;vvy;v;l;,,;.,,..,   v88v  hYv:  ;;::,..  .    .,:;;;Y  ., ,j0jb22jbj2tbj2jpppjbjpjbj2jpjbjbjbj2j2jbjpj2p2jbj2j2j2
0Cq00CqC@0#0#@Z0##DZODODOO9D9O9O9OGOGOG9G9GGGGG9gGe6egBgeGe6eeBeeeBeege6egGO6Gegee8eeOO98eg9G6BBB6Bee6eZ9BOceOZBebDB8ZcccUrlYqcAhyMUbC##999ODq#qqCqcmtOGeOOqD#69O#O9GO9OGOGOOO6egGeOqjZcXkjcUhAyy;:lvy;vvvv;;;;;;.,   vB8c. ,V,,, :v;.:.,,;:.   :;;.   .:,;v;vbCbjbjbjpjbjpj2jbj2jbtpjbjbjpjpjpj2j2jbj2jbpbjbj2jbj2j
C0CqC#0Z0@0ZqZ0#qDZDZODDZOO9O9OGO9OGOG969G9g96G69G9eeegeGe6eeBee6ege6e6e6e6egBeegB99DOGBgDZgee69O6eB9gBG98e@9e2B6@2C6BB8BOjO4UwwUXrAAyrryTwTTkhMyMX0ZOZZ@@j0qG9G9GgegeGG96GGOG9eeBee6eGOCbccHXryvv;vvl;v;vvvv;:;,,   VB8ml;;l;   ,Yv,,.,.:.  .rl ..  . .;cHctqqbtjjCjbj2jpjpj2jbjbjbjbjbjbjbjpjptbjbtptbj2jbj2jbppjb
qCqC@CqC#qZ0Zq@qZ@OZODODOZ9OOO9OGOGO9OG9G9G9GGgG69g9gGegege6BgBeeeBee6e6eeBeB6g9699OGGBeGCZeee6ZGeGOGGB660GG99C9eZpqO@0jptqDNt02OckTNcMLUNwywwryAhpOZHcpttq#ge8B8B8BBeegeg9O9OGGG9GG6Ge6G02pCmcmhYAvlvv;vvvvv;, .,   m8jwXvvr.   ,L;,.:    ;pB8r.,. ,  .vCBCO0Cjptbp2tpj2j2jbj2jpjbjpt2j2t2tbjbjbj2jpj2jbjbjbjbjbjbj
C0C00qC@0@q@0#0#0ZZDDODOOOO9O9OO99OGO9GG969G9G9gGeGG96Ge6e6BeBeeeeeeGe6eeeGeee9G9696Ge9Oq9BBZg9O6e9egBgeG6DeBOUAO8bkC2HHkkc0TjXYvMlVVlA;:: . ..vy4kTrcbbjOGeGeee6e6eeeeBeegegege6eGG9GegOe9OOZCqtwvY;vAlvvv;,;. ;r,   #OqL.hl. ,,;:.   .L2e88Z:.;,    :kvhD0bpjbjbjbjbj2jpjpjbjpjbtpjpt2jpjpjbjpjpjbj2j2tpjbj2jpj2j2
qC@C#Cq0Z0@0ZqZ0Z@ZZOZOD9DOD9OOOOO9O9O69696969g9GG69gGG6eGege6BeBeBeBeeee66Ge6eGOOO9GDO96GDb0Z#qGDgBegBge9g9GODj2ZCk0CpHUwvHcvyhMALv;vXv;;lv..;vVvvvUtcq0bZ#GeBeegBeege6e6BeBeBgeeeg699D9Gee6e8OtkjUylVvvv;,. ,ly;    ;Hk.;T; ,;.   Y0BB8B8qV..:.     kelVDp2j2t2jbjpjpj2jpj2j2jpj2jbj2jptbj2jbj2jbjbjpjbjbjpj2jpjpp
CqC0q@CZ0#0@CZ#Z#DqDZDZOD9ZOO9O9DOO9OGOGOG9GOG969696OG9egegeeeeegeeBeBgeGeGegeG6OG969GOGGO@DqD#96OD8ODgg0qZO#999cmjbjZCtUkArNvlvvv;l;vvvv;;;vv;vvvvXt#0tUj@O9egBeege6e6egeeBeege6eeee8B8eO0##OO9Z#qHVLAMvA; .vlY:    :,v,;v,     .VBB86ejMv. ,;:    .XOCvYqCtpp2jpj2j2j2jpj2jbtpjpjbjbj2jbj2j2jpjbj2jbjpjpjpjbp2j2j2
qb@0@C#0@qZqZ0D@ZqDqZ#DZODODOZOOODOOGOGOG96969G969e969ggeGeGege6BeeeBeeGegBgeGG9O9Oqe6GGe9OCq#ZOeG9G#OeCGObcq0@CDD02cAmmUmcLAMw;lv.vlv;;lvv;vvvvYrUjC2tktbOO6eeeegeee6eeeeB6e6Bee6eeBeeeeGGO90#Z044XNrAy4y.;4v,,:     ;;v;     MBeOHV;;.,    ..    vGe@MvvC0jj2j2jbjpjbjbjpjbjbjpjpjpjbjbtbj2jpjbj2t2jbjbjpjpjpj2jpj
CqC00q0Zq#qZqO@D#DqZqZZDZOO9D9DDOOD9O9O99G9GOG9GOG96G6GGGg6eGegeeBeBeB6e6BG69G99ZGOGeG9g@jCOCOZOOO#O#e9b9Ob#Z9bC0CjcUUUUMAvUkNrAyrVv,v;vvvvvVArkc2bm0pHttZ9e6B6egeeeeege6BeeeBeB6e6eGeGg6e9GOg6eGGbkVVXGY.rw  .      . .     UB8BeqbtpjtjptUyY;;;vUGBgmv:A@btjt2j2jptbj2jpjpjpj2jpj2j2jbj2t2j2t2jbjbj2j2j2j2t2j2jbjp
qC0Cq0#q@0#@Z@ZqZ@Z#ZqDZODDZOZDD9D9O9OOO9O9O9O696O6G6G6969e6e6egeeBeBge6egG9GO6OG9BeeGgGZCO#ZO#pDZ@CpCCCZjpppHUctctwpHAUCHtkMYAyAAhYv;vvvvrAUwXcCmk2jUcpCZG9e6e6egegege6eee6eeBeBeeeBeeeBeeGB666eDchjZCtVrM  :        .   ,qg6eO9#8Ztq#j0ZDCZqjmjtmc2tL:vjOc2t2j2tpjbt2j2jbtbjpjpj2j2j2jbjpjpjbjpj2jpj2jpjbj2jbjbj2j
CqC@Cqq#q#qZqZ@Z@Zq#@Z#OZOqZZOD9O9OOOOD9D9OOO999969g96G6969G6eeege6eGeee669e96GGe6GeD66eOOO@pZ@#DDCcHOCb2pj02tVmcHcHXXpbYUmhYrkUVvyvrrYrVrrTAUkH4UkcXUc0bOGeGe6e6eeB6BgeeegeeBeBBBeBeBeBB8Be6G9BBe8e9ZcDw  ,y  ,, .     :OB8bZUcTk@mjOUHAhrUUUhXT4MMAV:,LZbtj2jbpbpCtbjpj2j2j2j2j2tpjbj2j2jbjbtbpCjpjbjbjbjpjpjbp2jp
qCqC@0@0Z@ZqZ@ZqZ@O#ZqDZOZD#OZOZOZODOOOO9O9O9O9OGOG9GG6GG9gGe6e6e6eeeeeee9ege6eG6O6GG9BOZO6jCDOC0mbHH#pcCpk4ccHcjcmyUjHwcNrmwUAvyrlvVvYvvlYrcHkhkHHUHHC29GeeBeege6BeegBeBee6eeBeBeBeBBBB8eBeBeBe8B8D9ej;  ,v   :  ;    .BjcTHtccrXyvUvvlv:,Nl;v;vVrvvvv;jqptppCp2j2jbjbj2jCjbp2jbjbjbpCj2jbjjjbjbj2j2j2j2jbjpjbjpj2j
CqCq0#0#qZ@Z0Z#Z@D#D@D#DZOZODDDO#OZOOO#OO9OgOGO9OG9G9GO69G9gGe6e6e6e6eGe6e6Bge66OG9GGeZZZOZ@0DCCbptUcmmjkmrm0ZcbCpcXkkrvhAyHyhvvAYvMYvvv;VU#ZcT4kHNMUppOee6e6B6ege6B6egegBge6egeeBeBeBeBeBeBGeeO68BOL;  ;yv    ,  .   :ek4ckTcN;lUvvyv.,.  rv;:;,;v;,:;YCqtjtjtpjpt2j2jbj2jbp2jbp2jbjpjpj2j2cjtjj2j2j2j2jpjpjppbj2jp
@C0C@0Z0D@#qZqZqZ#O#DZOZDZDZDZOZDZODODOZDO999O9O9OG9GOGOG9G969gGe6e6eGeGe6e6eGegGO69GOO9G@0ZOC##@bbmOcjHHjpCcUjLlvlUmrCjmAhUHmwhcUMv;;hmHHjXkAAHmrMTkjOeBeeGeGegeeBeeeegeeB6Bge6BeBeBeBeBeBeegeG8py  ;VV4v;   ,,      9jvXjcH4VyHL;;;rvv  , ;. .,., ..v@D2jtptptjtjtjtptjcjtptjtjtjtjtjcjcjtbj2jbj2jpjbj2jpjpj2jbjbj
C0CqqZ@###qZ@Z#ZZDZDZDZOZO#D#OZO#O#OZDDOZOZOOOO9OG96OGO99GOGO9969g6eGe6e6B6e6geeOGee99OO##qO9C0GCpkCCmtppZthvm2V;NmhYXmthhXXTyrXyyXVAjtVyNHw;VTMyyyUm@eBGg6eee6eeeeBgegeeB6e6BeeeBeBeBeeeBeBB8B8U  AbkXq4vy;,;v,     ,ZA4pUcAVXMVXvl.;v .j;L; .,....,,:D#ptjtjtjtptjtjjpjjjptjtjtjtjtjtpjjtptjt2pCjbjbj2jptpjbj2jpjp
@CZ0Z0Z@ZqZqZ#OZD#D#DZO#OZO#OZOZDZODOZOZOZO#DO9O9OOZ9OG969G969G9G9e6eeBeegBge6eee9GggZOO9#O9Gq#ZDCCjmtbjH4HyV@pUUHGvhqAV;AchyLHNArkcjTrMUV;;rVyVAwNUj#eee6e6Bee6eee6e6e6e6egeeegBeee8eeBBBBBB9t  X8BbmepVwY;v:v      qZMXkrUylAVvvvYV;  rHAT, .....,;  tDjjtjjjtjcjcjcptjtjtptjtjcjtjtjtptjtpjpjbjpj2jbjbj2t2jbjbjpj
0qqZqZq##Z@ZqZZD#OZOZDZDZOZOZOZODOZOZOZOZOZO#OOGOODOD9O9OG9GG69GG6Gg6egeGeee6eGee9Og6OO9gDDG2Z0cZjmHUtcmpkjtkUH@rvcvYVvNvr4llbcvvHpM;;vv;:vkjLVyvrwkj9GBeegegege6egeeegeGeGege6BGegeGeB8eecNy,.kB8BecCjccpy;,::      pUwYmhwlv:Yvv,.:, yBml  .   .,;:  qZjtptjtjtjcptjtjtjtjtjcjtjtjtjtjcjtjtpcptjjbp2ppjpt2tpt2j2j2
@CZqZ0Z@OqD0ZZO#D#Z#DZOZOZO#OZOZOZOZOZDZOZOOOOOD9O9Z9O9OOOG9GG6G69G99OG9e6eGe6e669GG6DO#OqDqOCGCpmC0tUjbwM0UUvrwYvwvvvwXrlAvvrL;Vyl;;;;vyyyv;lYvlrXkCOeeege6eGe6egeeeeegeGe6e6egg6B6GD9#ccjCg98G9O9Z2DOC0cV.:;:.    .tNUAhNArvrv;;;:; ,B#v;  .:.,:v:   CDtjtjtjcptjtptjcjcjtjtjcjcjcjtjtjtjcptjtjtptpjpjbjpjptbjbjbj
q#q#qZq#@ZqZ@OZOZOZDZOZOZOZO#OZODOZOZOZDZOO9OO#O9GOODODOO99GOGO9O6G6GeG69G966e66OgG6O9O#ZDZq@qO0t9pcMkbMvtCrMNr@cLrl;UULyvvvyr;vVY;,,vTwlv;;yMvvrMcjj#9Beeee6egeeeeeeB6eGe6eggGgGegGZqqbHeB8Bepm6g2G@OOOpMv;;v;.    MjXTryvwHTwv:.,:  98v .,,:;v;;,    b#jtjcttptjcjtjcptjcptjcjtptjtjcjcjtjtptjtjjjtjtjjbj2jbj2jbjb
#q#@#qZZDZOqZ#OZOZO#OZDZO#OZO#DZOZOZOZD#DZOZODDZO#9OGO9DGO9OOOG9G96Oegg96969gG6969eG699#g0Zbp@DCb@tCpj0whBmvCmvUjyvlvkl,::vlVY;vYv;;V;::vlrVv,vrkmccbq99e6egeeeee6egBee6eeeGe6e6eGGOG9ee8BeDODeBeqOGGO9qkAUYMy;,    qkmANyVAVllvA;,. v8y..,,;;;;;,.    cOtjctcjtjttcjttcjtjcjtjcttjcjtjtttjtjcjtjtjtjtjtpj2j2j2j2t2t
q#@Z#OZOZD#OZOZD#D#OZOZOZO#OZOZOZODODOZOZOZOOOZD###DDODOD9O9O996999G96GGG6G6GG9e9gGe9GqZZ0CDODtCjCbqcckwwZ4lUUkcyvrkAv::lrvl,,;v.,vyvvvv;v;;:;LUUHjCqeBeGe6e6e6e6e6egeee6eGe6e6g969ggBGgeeO9Geee96OeG9ODtHkhwvv:    cwNyrXTY;yvyv;,. ve, ;,v;;;:.      jOpcjctcjtjcttttjtjtjcjtjcjtjtjtjcjcjtjtjcjcjcjtjt2jbjbj2jbt2
#q#qDqOZZ#OZDZD#OZO#O#OZOZOZOZOZOZOZODOZOZDDOZD#D#OO9OGO9O9O9O6969GOG9G9gG6Gg96G6G69GOOOZbCqGtkcbcjkHHUwMtAYhTywLlh;v:rwLv;; .;v;:v,,;..;vv,;L4UHmbpD68geGBgeee6eeegegeeegeee6eG99ee699OG96G69G96ggG99gqjjp4hlv,    tyryXNv.;v;;..;, vB, :,;;:.        cOjjtjtjcjcjtjcjtjtjtttjtjtttjcjtjtjtjtptptjtjtjcpt2j2tbj2tpj
@Z#ZqZ#OqD#OZOZOZOZOZDZOZDZOZD#DZDZOZOZOZOZ9#Z#O#OO9ZOD9O9O9OGO9O9OG9GOG96GgGgGe9G9GOGZZe#DOttj##CHtmcmrycvVrMApY,V;Vv;V,:vYvVvV;;;;:, :;;:;vcXU4CjOeeGege6e6eeeGegeee6eeBeege6egBe6O9969gge96ee6gG6ZG9tjOmcjv;v    XYLyyl;vv;,,   , vZ:  ..           cDpcjtjcttjcjcjtjcjtjtjcjtjtjcjcjcjtjtjtptjtjtjtjtpt2tbjbjptb
Z@ZqZqDZZqO#OZOZODOZDZOZDZOZOZOZOZOZOZD#O#DZOZOZDZZZOOO@OOOOOO9OGO9OOOG9G9gG6G6GG9gGepZZGZO#jCqc2cpccctAAAT4MyvMVvwwU;lv,vl;v;;.;;:,:;;vv:lwXXkNZ2bBGDZOe969GOgGegBge6e6eeeeBgeGegG9G9gG69gg69GG6GgG9@@cZCmtbvv,    HUAlv;lvyvv;;  . YOv. .            mOjjcjcjcjctcjcjcjcjtjctctcjcjtjcjtjcjtjcjcptptptptjcjt2jbjbp
qZ@ZqOZOqDZOZDZODOZOZODDZDZOZODDZOZODOZOZOZOZOZD#OC@ZOO9OODOO9OGO9O9O9OGOGGe96Ge96e66qOGZZCqtqCjcjHtjmAlywAwrYyVlXYvVvl;;v;;vl,.v,;v,;;v;;LMTA4ZGC@9D2CB999gGeGegegeGege6ege6e6e9G9GGeG66eGe9G969e9eOOC0XcC2UV..,   ,r;;V;,:V;:v, ,. r#k:              HZptjtjcttjcjcjcttjcjcjcjcjtjcjtjcjcjcjtjtjtptpjptjtptjjpj2jC
#qZqD#DZD#DDOZOZOZOZOZODOZDZODOZODOZOZDZODOZOZOZOZO@#qOO9DODOZGOOOGO9OOO9OGGG96GgGeqGOGO#2q@qj02jtjcmNMA4TAylywvMrv;yv;v;yvv;;;;;,;,;;V,;Yw4UV0gYLBttZCCDZGGe6eGgGe6e6e6e6ege6g6G9e6egeGe6ege6eGgG6G9ZD@C9GkAYvv;.   vhv  vy..;.,,.  Vcc;              HZtjcjcjcjctctcjctctcjtjcjcjtjcjttcjtjtjtttjtjcpcjtjtjtjtpt2j
@Z#O#OZOZZ#OD9DODOZOZOZOZOZDZOZOZDZOZDZDZOZOZOZOZO9OCq#ODOZOZOO9O9OGOODOOOOOOg9e9eOC#99O2j2D#jbZccpmwkkU4HAyvAVlrl;yyv.:,;;v,;,;;v;;;;;;yvvX:cjwAmBcNcC0#9O69e6e6e9e6e6e6e6eGeGG6eeB6e6eGe6e6e6eGg96OODGOOppwv;;;.   . vv;,  ;.,     vct:              ZZtctmjcjtjcjttcjctcjtjttcjtjcttptjtjcjtjcjtjcjtjcjcptjjjjptp
ZqO#OZD#OZODOOOZOZOZOZOZOZODOZOZODOZODDZODDZOZOZOZODO@ZqOZOD9O9OOZO99O9DOZ9OGOGGGGZb9ZO0b2#GZt22qHUtjUk4NkVvTMllvvXv,:v;vv:,,;;;:;v;;vvvVvUL;bUvqqCe0r#gG@eG6eBGGggOGOege96gege6eeB66OG9eeegBee6eg69G99C#@Cjyl;;v     ,U: , ;;...    .bk,             :90cjctctctctcjcjcjcjcjcjctcjcjcjcttjcttjtjtjcjcjtjcjtjt2tptpt
#Z#Z#D#ODOZOZOZOO9ZOZOZOZOZOZOZOZOZOZODOZODOZOZODODOZD##ZOZODO#GOZZ99GO9O9OGO9O9eOpZGDOObZGOqZtCCmmbHU4MATvAhYvy;c4;;v:;vv.,;;:,;v;;;v;v;vrVHlmLhUyjcjZq9ZCGG6G69gG9OGeB69Geee6B6eGG9G9G6e6BeBge6eG6O9#C0ZZcrkvv;.     ,;;;, ,        cX              v9ptctctctctctctctctctctctcjctctttcjctcjtjtjcjcjtjtjcjcjtjtptj
DZOZOZODODOZOD9O9ZOZOZOZOZOZOZODODOZOZOOOZODOZOZDZOZDZD@OZOZD#DO9ZO9GO9O9O9O9OG9G@@OO@9j2Z#bO#bmCcckN4mAThANAvrrlrv;v;vv;vYv;:;;v::;vl;;VyvTy;m4chUcHtZc6ODG8GOD6OGG6GegeGe6e6eGeG696ee6e6e6BeegegeO9O9q@CqwTw.;v.      .    .       .pH.             L9bctctctcjctcjctctcjcjctctctctcjcjcjcjtjcjtjtjcjcptjcpcpjjtpt
ZODODOZO#DDDZOD9OODDOOZOOODOZODOOOOOOOZOZODODOZODDZDZO#ZZOZOZOZ9ZZDGDOZODOO9OGOG2qG6C###qZCZbCcHtpkmXXNhTUXUyrX4y;vy;YV;;l;::;::;;v;:AvlwVctvXckmULZ2cZbqgO6g90OOODeG696GeeeGeGGeeOGGBeegeeeeB6e6e6GOGGG##HcwV;y,.                    c#:             wGtjcjcjctctcjcjcjcjctctctcjctctctcttttjcjtjtjtjtjtjtjtjtptjtp
OZOZOZODOD9ZOD9OOO9OOZODOZOZOZODODOZOZOZODODODODOZDZO#OZDZOZO#Z@OZDZOZOO9O9O9O9O0@GOZCOOO@Z@bp2cqwctUT4AkAM4MyAvvvvvY;lv:;;,:;v;v;;;rvVLyht;pNThwDkH@mcZ9OOG9@OO9D6eGGBOGge6eGg6eGeGeGegBgBge6e6egeG99g6Z#cmk;y;:.        ..          jq,             mZtctctmjcjctccmtccctctctcjctctcjctcjcjcjcttjtjcjcjtjtptjtjtjc
ZDZODOZOO9ZOOOO9O9OODODOZODOO9DOZOZOZODOOOO9ZOZDZO#OZDZOZOZDZOZDZO#ZZODOD9O9DOO9ZDO9#qDOD9ZtcDmCHH2khmU4hTMkHyAVvvvLvvvv:;;v;;;l;;vVlvvvrU;wtVHAXCUkpmtCZ9@GGg#DOgG69eGG9e6eGg6e9GGegegeeeee6eeegeGG96OO9OcHhvw;.,       ,    .      .kq,            .pZmtmcmcctctctmtctcccjcjcjcjcjcttjcjcjcjtjcttjctctcptpjptptjtj
ODOZODOZODODOZ9OOO9OOZODOZODODOZOZOZOZOD9D9OOZODOZDZDZDDOZOZOZOZOZOZO#OOOZOO9D9OGO9#D0G9OtpC2mpcjpHckrwwctAyHyyvLv;yr;,;yv;vv;;v;vyvYr;yhYhOmtXvjcUCCmG2ZegOeG#@ggeDGO996O6GeGeg6GeeBBege6egege6ege6gGO@9ZpkHwh;:,                  .:tp             VO2cmtccmtctctctmcctmjctctctctctcjcjcjcttjcttjcjcjcjtjtptjcjtjt
DOZOZOZODOZOZOZ9D9OODOZOZOZODODOZOZOZOZODOZOZOZOZOZOZOZOZOZOZODOZZZ9ZDZOD9D9Z9OOOGZOD9OO@CbCpCmcOtAUHmH2cUyNHTvvMT;Avrv:;yvv;;vvvvVTvlAlrXX@YvUNCUrCOpZ6jOOCGOZ9O9O999DeGG9e6egeeeeBee6e6egeee6egeee9G9GGDbbUNv.;,         ;         .UC,           :ZCjctmcmcctmcmtmtmtctctctctctcjctcjcjtjcjcjtjcjcjtjtjtptpcjcjtp
OODZODOD9OODOOOOOO9DODOOODOOOD9OODOOODOZOZOZOZOZOZOZO#DZO#OZD#OZD#OOODOOOO9ZZO9#9OOZDZO20#qjbjbpckkkjtjmXHVyUAAVNhlLVvvvY;;vl;vlvvAAv;trlYAbvvCHk26AHD0#ZD0t6D9OOD996O6GeG66e6e6BeBeBeBee6e6eeeeeee6G969BCcjwvv.;.                    Ak,           cqtjcmcmtmtmtctmcmtmcctmtcjctctcjcttjcttjtjtjctcjcjcpcjtptpcptjc
Z9OODOD9O9O9D9O9DOO9DOO9OOO9O9DOOODODOZOZOZDOO#Z#OZOZDZOZDZDZOZDO9OOZDZOZOOOZ9OD#O#D#ZZbCDbbcjj2cttpHtjAYkXwNwlUrlv;ylNYv;vy;yvvvMVYk4VYwMUjyyAmm#j4N2#GcCDtZO#9#ODGGeGG9GGg6egeeBeBgBeBeB6ege6egege66O9OZUjmATv;:                    Vt;          vO@ccmcmcmcmcctmcmtmcmtctctcjctctctcjcjcjcjcjtjtjtjtjtjcjtjtptjtj
ODOZOZOD9OOOOD9DOOODOO9OODGOOOODOZODODOOOZD#OZOZO#OZOZOZOZDZO#OZOOOZOZOZD#9ZZO9#9O9Z9OZ0ZC2C2jbtptjHHcjX4mVyvyyyMlv;wvlvXYvyvyyyAryvMrAXVwrkAtmlHZT#ZwpDmc9j@@DD99G966e6e6eGegBeeeeeBeBeeee6eee6eeB6e9G9GGb@Zck;;.                    vCv          mZtjctccmtmcmtccccmtmcmtmjcjctctctcjtjtjcjtjtjcjtjtjcjtjtjcjtjcjc
OOOOZODOO9O9OOOOD9D9OOOOD9O9OOO9ZOZOOOZO#OZO#DD9O9DOZOZOZOZDDOOOZZZGOGOOZOOD09OZOOOOqDq@#qcqCCpptjtkcjMHmkyAyAYrVyw4vyvvUyvrAYvVhYAUvlkUvHyUccH4hbtCGprmCCqOZZO99eee9G6egBGeeBeBeBgBeBgB6ege6egegeee6e96OOq9tkv::,.                   v0:         vOjcmtctmcccccmtctmtmtmcctcjtjcjcjcttjcjtttjtjtpcttjtjcjcjcjtjtjcj
9OOOOOOOOOODODOO9OODOO9OOOOO9O9O9O9OOZ9O9DOZD#OOOZODODODDZOZOZOZOZDZOZODOZOZODOZGO9Z0Z6ZO@bjCtbbCmcpHkkXmMcryXYwNYMvvUwvlVyVUVlvVvVccrcwwmwy4jHbkU#0DtktOqOOOO6GgGege6egegBeBeBeBeeeBeBgBeeeeeBeBeBgeGg9DCG@UVrv;                     Vj.         q#tmcmtctmtmcmtmtmcccmtmtmcmcctcjcjcjtjcjcjctttctttcjcjcjcjtjcjcpc
O9OOD9OOOOO9D9O9O9O9OOOOOOO9O9O9O9O9DOO9OODOZDZOZDDODODOZODOZOZOOOZDZODOZOO9O9D9ZODO#9CObC@qC0pCtmmjhHmccMAwAkvrTAMyVyvkYvYwLVwAvrLk4hXTwHArkHA0j4UqCtCDZDOOGeG696Gg6eGe6eeBeBeBeBeBeeeBeBeBeBeeeB6eg69GOGOCwwml;:,                   XH         rqbctmtccccmcmtmcmtmcmtmtccctmjcjctcttjtjcjtjcjtjcjcjtjcjtjtjcjcjtj
OOOOOO9O9O9OOD9O9O9O9OOO9OOO9O9O9O9OOOODODOOOZOZOODZOZOZOZOOOOODOZDZDZOZO#OO9OOOGOZqOC#DD#9Z@jptckmkUjtHpUrrMArrAvyLk4vhXrrlYlryXwAUkLrHUrCmkkHmt2t#Z0DZOO996g69e6ege6BeBeBeBeBeBeBgB6BeeeBeBeBeBeege669eO#0UkX;;:.                   cL        MZbctHtmtmtccmcmcmcmtccmcmcmcctctctcjcjcjcjcttjtjcjcjtjtjcjcjtjtjcjc
O9O9O9O9O9OOO9O9O9OGO9OGOGO9O9D9O9OOO9ZODOO9ZZ#ZD9DOZOZOZDZ9O9DOZOZODODODOZODOOOZO0Z9OZZZDpC#@pqC2mkmDc4UmAXrwyrrVlVhlAVlkAUVVUrVhUUNmk44yAjtkUktOC#OGZ99G969GGg6e6BeBeBeBeBeBeBeBeeeeeegBeeeBeeee6eGe966q09CUv;;;                    HY       49Cctmtctccctmcmcctmcmtccmcmtctmjctctcttjtjcjctctcjtjtjtjtjtjcjcjtjtt
OO9O9O9OGO9O9O9O9O9OGO9OG99OOO9ZOO9O9O9DOZOOOZDZOO9OOZ9OOZO#DZODODODD#OZODOOODODOOOOODOCODbCD0j2jktjCkUM4HwwNywrYymlAv4yvrNN4vkchThTActMUHyLmUmc2q9O9G6Gegegege6BeBBBeBeBeBeBeBeBeBeBeBeBeBeeeBgBeBeeGG9e#9#jrVl;,.                  .k:      yOCmcmtmcctmtmtccmtccmcmcmtmtccmttjcjcjctcjctcjcjcjttcjcjcjtjtjtjtjcjt
OOO9O9O9OGO9O9OGO9O9O9O9O9O9O9OOO9OOO9OOZOZ9Z9O9Z#ZDZDOG#DZ9O9DOOOD9ZOZDZOOOZOZOO9OOZO0Oq#OqCqjqUXC@twymmUAAkhrAYVhvvrUyywVryYywXUwXAUcXrjhrmjkj#OOG96gegBeBeBeBBBe8eBeBeBeBeBeBeBeBeBeBeBeBeeeBeBeBe69gGqC2mtrVv,                   vl      k92cjmcmtmtccctmcmcccmtmtmcccccmtmjctctcjctctcjttctttcttjcjcjtjcjtjcjct
9O9OGO9O6OG9G9GOGOGOG9GO69GO9O9O9O9O9OODOZOO9O9OD@Z@ODOOO#DZOZODOZOZODOD9OODODOZ9DOZOZZDDqDbjjjpcjCcHMwcp4rrklArrrhrlvrvrM4wkhhMhhcUkyV4MwmmHHmcOO66egBB8BBeBBBeBBBeBeBeBeBeBeeeBeBeBeBeBeBeBgeeeeBee9GOgO#eOUwv; .                  v      A9Zkbccmcmcccmcmtccmcmtmtmcmcccmtmjctcjcjctctcjcjtjttctcjcptjtjtjtjcttjt
OO9G9G9G9G9G9G9G969G96969G99O9OGO9O9O99OZOOOOOZOOGO9O9ZZD9DO#DZODOODOOO9O9DOZOZOOOOODOODDDpC2ZjjjZbtAhXANUVXXyylvLMMAVlrrwrlrrrkwwrrmUYHAAkHkHpODgeeeBB8eBBBeBeBeBeeeBeBeBeBeBeB6ege6eeBeBeBeBeeeBee6G9GO6OGcUw; .,                 ;v     cO2HcmHHcmcccmcmtmtmcmcmcccmtccmtmtctcjctctctctcjcjcjctmjtjtjcttjtjtjcjct
9969G9GOGOG999G96OGO9OGOGO9O9O9OOD9O999O9O9OD@DZOO9ZDOGZZZOO9O9DZ@OOOZOO9O9O9O9OZZ9OGOOO0pZ@#HZCcctchUHVUUrArVArVlYryvyLAlAAMrAywwwVyMTAMMUX4H@9e6egBBBeBeegeGBeBeBeBBBeBeeeeeBgege6e6egeeBeBeBeBeBge9699OOqpHU;;yv                 Y;    rO#mcccmcmcmtmcmcmcmtctmcctccccmtccmtmtctctctctctcjcjcjctcjcjtttjttcjcjtjt
O6969G96OG9GOGOGOG9GO99G9GO9O9OO9GO9O9O9O9OOO9DZ##O9ZOZDO9ZO#OO9q#Z9DOD9D9O9O9O9Z9O9OOCO00ZZtpCCmkccTkTA4TVAVvyAUy;HXLvVyrVrMU44rUHUAyAAyANmHbOBBBB8BB6e6ege6eGgGe6BeegeeBeBeege6ege6e6e6BeBeBeeeBee669GZZ9etwkvvl,                 l    HOjmcmcmtmtmcmcccmcmtctmcccmtmcmtccccccctctccctctcjcjcjcjcttjcjtjtjtjcjtjtj
G9G96OGO69GG69G9G9GOG9GG6GG9G9G99O9O9D9O9O9DGOGODC9OO@DO9#D#9OZZ99Oq9OOOOOOOOOODOO9O9OOtDDCC#p00HHjHwhrAMLwVVvMVvyVlYlYvyTlyAvy4yvyrAVhArMHjOG8B8B8eB6e6eGeGeG69e6eeeGe6eeeee6e6e6eGeGe6eeBeBeeeBeege6gGe6GbjcAv;;.                ;;   tGjccmcmtmtccmcmtmcmtmtctctctmtctmcmtmcmcccmtcjctctcjcttjctctcttjcjcjcjtjcjc
9GOG9G9G969G9G9G9GO69G969G9GOG96O9OG9GOOOOO9O9O9ZDO9ZZ@GgG##@ODOO9OOOOOOD9DOO9O9O9O9DOqZZZ22bOHHmU4kkmyyALrryllyVLVrvVyylVvyyLVArLVAyllLAc#gB8BBe8ee96GeGe6gG6Ge9G9G9ege6B6eee6e6eGeGe6egegeeeeBeBge6e9gDe2CeDwvy;..               V  :tOpmtmcmtmcmcmtmcmtccmcmcmcccmcmcmcmtctmcccccccmtctctcttjtjcjtjtjtjctcttjcjcj
G9G9GO6G69GOGO6OGOG9G969G9G9GOGOOOG9GO9OOO9O9DZ#99GDDOOCDZ9ZOZDOGG9Z9O9D9OO#OD9OG9GOOZDObqgjOC4HjyAmHXUvwyMLyr;;rvVVvVVVvvrvvVrvYYAvYVA4jqGe8B8eBBBeeGeGeGG96O69GGGGG96GeGeGeGe6eeege6egeee6BeBeBeege969OOO6BUvlU,l,              v. .O#jmtHtmtccmcmtccmcmcmcmtccmcmcmcmcccmtmcmcmtctccccctctcjttctcjtjtjttcjcjcttjc
9GO99G9696969G969G9G9G969GO6OGOGOG9GO99gO9OOG9OOZZO9O6OD@ZO9OOOOZZZOO9OOO9O9D9O9O9OGO9#ZCOOtqHmDHwUmrMAlyyrYlrlvvvvv;vvvvvlvvVyVvvvvl4UjqOOBB8eBeBeBeBge6e6e6e6eGegeGgGgge6egegeGe6eGeGegeeege6eeeee6G9GeeO9GjLVvvv              :  ;00jmcHcHcHcmcmcHcmcHcmcmtmcmcmtctmtmcmcmtmtmcmcmtmcmtctctcttjttcttttjtjtjctcjcj
GGGOG9G9G96969GOG9GOGO69G9G9G9GOOO6OOO9O9O9O#DeGG#ZO6O9OGZZ#96GDZ#OO9O9O9O9O9OOOOD9O9#ZZOC2CjHpmUmmAAVrYVyr;vvv;lvvvvvvvvvvvvvlvVlyvVwUcZOgeBeBeBeBeBeBBeee6e6eGeGe9GG696Ge96G69G9ege6e6BgBeB6eeeeBeeG69e9GO#Trv4,  .           ,  :GqjccmcmcmtHcmcmcHcHcHtmtmcmcmcmcctmcmtccmcmcmtmcmcmtctcjcjcjctttcjcjcjtjtjtjtjc
O6O69G9g96969696969G9G96OG9G9G9GOG969ODODG9GOOZOO6@OOD#DD9O9ODD9OGOOOODOO9OGO9O9DOOO@ZOO@ZC0t0khHUMwyMryvyV;vl;;vvv;;v;vvv;lvvvvvVVlV4cqD998eBeBeBeBeBeBeBeBeBeegeGG96969G96G69GG6Gegeee6B6e6egeeeee6g96G9ZtXcbtk. .              vOjjccHcmtHcHmHcHcmcHcmcmtmtmcmtctmcmtccccmtccmtmtccccmcctctcjttcjcjctctcttjtjtjtj
G9G969G969G9G96969G9GOGOGG9OG96OG9G969696O9O69OZDZOO9OO#DZ9O9OOD9O9OOD9O9O9O9O9O9O9OO#O2C9O4mDcrHUAyyAMVLv;;Lv;;v;v;v;;;v;;vllvvryvAkkj2Oe8Bege6ege6eeBeBe8BBe8BBgegG9G99Og969G96O6969e6e6eeeeege6e6G96Ge@O9@2Oh,;,;,            vOqtHmmcHmHmHcmcHcHcHcHcHcmtmcccmcmcmcmcmtccmtmcmcmtmtmtctcjctcjctcjcjcjcjtjcjcjttt
9G9G96Gg9G9GGG969G969GOG969GOG9GOOO9O99699OG96OODOO9O699ZOO9O9DOOOOOO9O999DOOGO69GOOZZO2tG#hpqAAUyAhv4VvLv;;vv:;;;;;;vv;;vvv;yvvYLYhXmXqB8eegege6eee6eeegeeBeBeBeBeeGe6G9G9999O99G9G96G6Gee86e6eGeGeGgGgDO9G0O9Yv;.,            vDbccHHcHcHcHcmcHcHcHcHmmcHcmcccmcmcmcmtctmcmtccmtmtmtmcctctcjctctttcjcttjtjcjtttjcj
GOG9696GG9G9G9gG6G69G99969699OG9GO9OOZGO6O9OgG9D9Og9eOO#DD9GGOOOOO9O9O9O9O9O9OGO9O9OO#ZbOqU4jAUHwyMwyrv;vvV;;:;;;;;:v;;;vvl;vYvvAM4cUyce8OOBBge6eGeGg6e6e6egeeBeBeBBBeBGB669GOGO6O99GOGO69e6e6eGeGeG69G9O@99Deb,Av            .YGbHkmccHcmcHcmcHcHcHcHcHcHcHcmcmtmcmcccmtmtmcmtctmtmtmcmccjttctctcttjtjcjcjtjcjcttjc
OG9G9G969G9G9G9e969G96O6O69GO99G9699OO9G99O99GO9O9OOOOOO@ZOGO9O9O9OOOODOOOOGOOOOOGG9D9@bZZTUrmcUrAlylv;vl;;;,;;;:;;v;;;;vv;vlLvNU4yyVwBB#998ee6e999G9e6eGe6eeB6g6eeee8BBee6e96OGOG9GO9OG9G9G9GGeGG9g969GGD68DbvvAv ;         .v@pcmmmcHcHcHmmcmcHcmmHcHcccHcmcmcmcmcmtmtmtctmtccmcctmtmtmtmjttctcjtjtjtjtjcjtjtjtjct
G969G9g96GgG6GgGg9GG69G9696969G9999O69g9GO9D9O9O9O9OGO9OODODOO9O9OOZ9O9OOD9DOOOD996ODZOj22htkrHmLylM;vy;v;;vv;;;:;v;,.;:vvvvMyrckyyVXZgpCeBBB6eggOG96GeG9G888BBeBOGeeeBeBeBeeGGO6G6OOD99GOG9OO6eg9gG6O9D9ZGtt@LNV..;.        V#CcHcHmHmHmkcHmmcHcmHHcHcmtmcmtmcmcmcccmtccmcmcccmtmcmtccmcmcmcctcjctctctctcjctctctcjc
9GOG9gG6G6G6969696GgGg969G9G969GOG999GDOOG966GZDDeZOD9O9G69GD9O9DOO9O9O9OOO69G96OOOeZjbGtHtjrXjHvVll;lvV;;;vv,;;;v,.;;;;vYyVyTAUv;vXmZr4egeBeBe8ge96OO#OODCbCZB8BB9G66O9GBBBeeee969GOO99O9O9O6G69eGg9GDOOeebjeGc;,,,        LZ0cHmHcHmkmkmmcHcHmHcHmmtHcmcmcctmcmtmcmcccccmcmcmcmtmtctmtmcmcctctcjctctctcjctcjctctct
G9G96G6GgG69GGG969GG696GGOg969699OG96O9D9969e99D9O9O9D9OODOOG69O9O9O9O9O9O9OGOG9OO9ODjCOCcZwlwAYVLA;Avv;:;;:;;;;;,,;;;vvl;vvAwrVYvyLyyyj8eeeeeBBe6GODC0bHhrvv;vrCB8eg9GGeeBeBeBeB6e9g969GO9O69GOGGg9GO9DGGeOgDZc;l;,       V#cmmmkmHmHcHmHcHcHcmcHcHcmcmcmcmcmcmtmtmcmcmtccmtmtctmcmcccmcmcctctcjtjctctctctcjcjctctc
O69G9696GgGG9GGGGG9gGGG69G9GOG999G9G9GO9O6OG9699DOZ9O9O9OOD9O9O9O9O9OOD9OGO9O9OOOe9O#DqZOCUHUrrlYky;vv;;:;,;;;,;;;;;;;;vvvvyrrVTVvv;lcpeGeeegG9e6gO9Z0jjccUAvv;:;N#BGgeBeege6eeBe8eBgegeG69GG6OG9gGG9GO9#96OOClU:v:;;,    v@CmcckckmkmHckmmcHcHmHmmmmcmtctmcmtmcmcmcmcmcctmcmcmcmtctccctccmtctttctctctctcjcjctctcjct
G969GOGOG969G9GGG96OgGG96OG9GO9O69G99O9O9O6O99G999OC#OeO9O9O9DOO9O9O9O9O9O9OG969G9GG9#0C9kZkAAtYrUvvv;vv:,;;v;v:;vv,vv;vlvywULyVy;lj#CeBeeGD6@6GeeeBGZObtccMLyrrXXHcOO696969eeeBBeBgegBg69G6egeGegGGgGG99Z9O9mAhYv. . .  vCtccmHmHmHmHmkcHcHcHmHcHcmcmcmtmcmcmcHcmtmtccmcctmtctctctctmtmcmcmtctmtctctctctcjctcjctcjc
96969GOGO69GG6G69G9GG69G9G9G9GO99G9GOGO9O9OGO9966e9D@9GgOOOGO9G6O9ZOO9O9O9O9969699OgOD#0mjCUyjUVyA;;;;;v;;;v;v;v;vlvvVvlyAmXUwvvYUceOeeg0O#6e##ZOe@G9OO@cckUUHkmctc2bO9eGGOG969e9GG6geeBGGGeGeGeGe6eeeGeGOCqBBtmMv;: ,. yebtkHkcHmkcHmHmHmmmHmmmmmmcctctmcmcccmcmcmcmcmtmtmcccccccctmtmtmcmtmcmcmtcjctcjctctcjctctct
GOGOG969G9G96GG969696969G969G9G9GO69GO9OOO9O999OG996eZZ9GO9O999D9OOO9O9D9O9OG969GO99G@ZbCtbkhbALy;v;v;;;vvVvvvY;vlyUVYryUwyVNlAjb#Ge8Dt20Z00Op9ODCDDZCZCbccctmqZZcpOGO6eBG69G9696GeGe6BeeGeGggegegBee66GGgq4e8kcAV0v;v. y9pHHkmHmkmHmHmHcHmHcmcHcHcmcmtmcmcccmcmtmcmcmtmcmcmtmcmtmcmcmcmtmcmcmtctcptjctmjctctctctctc
9G9GOG9G9G9G9696G696G6969G9G9G99O9GGO9D9OOOGOOO9DZZG66OOO9O9OGOOOOO9O9O9O9O9OGOOO696ODqZCOkm0Ulry;v;y;vv;lvvvv;yvlyvvv;;;vrCOtGgb6e9b4bOm0@cHb#Hj8#OqCCDqp2CcHcbCDOg6BeBgBGe6G99GeeBeeBBgeGee6Oe669eeGZqZe#@OckklkL ,,  ;ZcmkmHckmHmHmkmHcHcmmHcHmHcmcmtmcmtmcmcmcmtmcmcmtmcctmtmtmtmtccmcctmtmtcccjctcjcjctctcjctct
GOG9G9G96969GG6GG9GG69696GG9G9OD9OG99OOD9O9O9O96eDZZ9O9OOO9O9OOD9O9O9OGGG99O9OOOO9g9O#ZDqCMkZVLwv.Mvv;vlvvVvl;Vv;vv:;vyrmG#ZDO6mtpt2UmgjkjqrHttk96jbO0C0#CDp#bp2ZOOj@G8egGBeBg6O9Oeee6e6eeeeBOOe89Z68GZ2G6q@0HZXUt; ... vOcHmHmHmHmkmkmHmHcHmmcHcHtmcmcmtmcmcmcmtctmcmcmtmcmcmtmtmtmcmcmcmtmtmtmtmtctmjttctcjcjctcjc
OGO6OGO69GOG969g96GG9G9GO6O99699G9OOO6OOO9O9O9#DO9OO@DO6O9OOO996O9OOOOO9O9O9O9O9O696OOZCZ0UcXLw4,Vwv;vvMYlvv;;;;;lvXkCqZ9@kZpqNrktjcymjUk0rw6jmqO0b@bD#C2q##OG@CCeOpj#9B88B8B8BB99O9OOOG966egGGBeegeGgGGqGD6O9HAA; ..:  vOHmHmHmHmHmHmkcHmHcmcHcmcmcmcmcmcmcmcmcmcmcccmcmcmcmcmcmcctctmtctmcmcctmtctctctctctctctctct
GOG9G9G9GO69G9G9G9G9G9G969O96G9OOO9O9O9O9OOOG99D9GOZOD9O9O99G99OGO9OOOOO9OG9G969gG9O999tbZHkrLvvhylvrlyvv;;:;;rkmU2CDjtCD2bjCkrttmtymNpUH#4Cew@9tt#CCqD@ZtZ0ZO@0O9OCjtC98Bee8BBBe6GOOOGOG9G9gG66eGBG99OC9Zcj0UMyV.;     rZtkcHmHmHcHckcHckcHcmcHmHcmcmcmtmtmcmcccmtmcmcmcmtmcmcmtctccmtmcmtmcmcctccmcccmjcjcjctctcjc
9G969GOG9G9GOG9G969G9G9G9G969G99#OOGOOOOO9O9ZOOOqOO9O9O9O9O9OGO9D9OeO9O9OGOG9G9GOGOODG@2ktHhrHvHUYyvArl;;v;lU#ZjcC0tH4jOtmHtUMkjkHmUUkccZbmqHm#cXtjpZ9CDCDttZOqZ99qjHmUpDg9eeB6eeB6G9GOG9GO99G9G9gGOD9O9BeHCXUm;;,      @CmHHmHmHmkmkcHmHmHmmcHcmcHcmcmtmcmcmcmcmcmcmtctmtmtmtmcccmtmtmtctccmtmcccccmtctcjctctcjcjct
G99OG969G9GO6OG969GO6OG9G99DOO6G9Z9DOD9O9O9O9D9OOOGO9O9O9OOZOO9O9OOO9OOO69G9G9G9OO9ZO9Op4UqyU4UATrNVyvv;YLkZ802cjtkcHMcmwmpMTcmkHkpAwD9ctAqtwO0wb#pmDZcbOZc0Gq0CZDDjjHkk20qZOOBeBB8eeOGOG9G9G99DGG6O66eOb@qm44yv.      YecmkmkmHmkmkmHmHmHmkmHcmcHmkmHmHcmtmcmtmcccmtmcmtccmcmcmtmcccmcctmcctmtmtccmcmcmjctcjcjtjtpc
O9OOOG9GOGOG9GOG9G969GO69G9GO9OgGeOGO9OOOOO9OGOODOOOOOO9Z99GOOZ9GgOODGOOO99G9G9G9ODg69tHUjcAjHyyVArM;vHHYqBOhXUUrwTc4UmchcHUmCHkUHTk26mVb0tUtOUt9jHGGpcO#Z#6q@OGCO2btc44kjmjcOBeg8B8BBGeG6Ge999G96OO9eZtCqCjwkr.       OjmkmkmHmHcHmHcHmHcHcmmHcHcHcHcmcmcmcmcmcmcmcmcmcmcmcmcccccccctmcmcmtmtmtctmcmtctmtmtctctcjct
9O9O9O9O9O9O9OGOGOG9GOG9G9699O9OOO9O9O9O9DOD69gOGOODOOG99O9O9DOOOOgG9O699DG969GGG9eOZ0DH2CmhbvrlYlrlwHjhO@HNyNmrUHHjHktcjp0mjUXXjwMbjMvhqDmmtkHjZkC6Zc0t0OqOOc@@OODCmHjkTyNAmtp0Zq6B8B8B8BB9GGeO6O9OO0b2@t0pAr;.,     kDcHmHmHckmHmkmHmkmkcHcHcHmHcHcmmmcmcmcHcHcHcmcmcmtmcmcccmcmcmtmtctctctmcmtctmcmcmtctctctmtccc
O9O9O9O9OOO9OG96969G9GOGOG9GO9O9O9OOO9O9OOO9OGOGO9OOOGG6O9OOOGO9DOO9D99g999696GG9eGDb#bjcHlArvyvMmkwk0CqCXV4UmV4Hw4kUj2cUccjHHXpjTXjTMctq8kkCG@tkqCOtcqCcGqqDqC@DG90c0bcl:;;vl;rkUv  ;X98888B896O9@GOb08DkHhyv;v:..  VOCHtmmHcHmHmHmHmHmHmHmmcHcHcHcHcHcmcmcmcmcmcHcmcmcmcctmtmcmtmtmtccctmcmtmcccccmcmtmcmcccmtmtct
9O9O9O9O9O9OG9G99OGOGO6969OOOO9O6GGO9OGG69G99O9O9O9O9O9D9O9OGG9O9OOD9O9O9O9O9O9OO96#qCk24rTAyrkUhXUbgObMhjmAUrHkhyj2tkhwccbHTHjtUr@Hcm206OtMt6qUcOOpkkqbZcZOCbmtGqZq@Hcy  .,;;:;v:.        vpB8BeeeDOG6qtkcMYvy;.,.v2bjHHkmHmHmHmHckckmHmHmHcHcHmHcHcHcmcmcmcHmmcHcHcHcmcmcmcmcctmcmcmtmtmcmcctccmtccmtccmcmtmtmtmtc
DOOOO9O9O9OG999G9G9G9G9696O9OGO9OOO9O9O9O9O99999O9OGO9ZOO9O9OGO9O9O9O9OOZOD9OOO9@OZqpCcjphwkTmTLTOO9OmULNmwrwUbXUkkckXUjck4HU4NkMjbNm0ZZCCjbD9bcCOtHH0#ZjC#ZqCbeBCCOUNy,   .,:,;,,::          ygB8eDbZ0mj2hLLyvYlAk0CckmkHkmkmHmkmHmkmHmHmkmmcHcHmmmHmmcHmmcmmHmHcmmmcHcmtmcmtmtctctmcmtccmtmcctmtmcmcmtccmcmcmtccct
9D9O9OOO9OG9G9GOGOG99O9O9OOOGO9O699O9D9O9O9OG99O9O9O9O9D9O9O9O699O9O9O9O9D9DODOOODZpCDbqZLkNMAN2BBpUUNyAmryXmNMLkUU4cjjkUXkwUrUHXttNjZCjZ#99@tbCGb#t0GOcDZj06bDCqt@jw4;     ,,;vvyv;;            k6DZ0CjmkcUv lb0C0mmHckHccHmkmHmHmHmHmkmHcHcHmmcmcHcmcmmHcHcHmmcHcHcmcmtmcmcccmtmcccmtmtctmcmtccmcmcctmtctmtmcctmtm
DOOODODOO9O9O6OG9GOG9G9G9GDODODOOG99O9O9O9OOO9O6OGO9O9OGOGOOO9969GOGG69gGe6g96O669OG#OcccmAvmOegq#wMTUkXNLAkHUrTHkhCUXNUTHwAXUU4Umcpbpm@UHZCtpC0DOO@OgC9G00Zp090cD92MU.     .:,;Vrvyy                         mCtcHcHmHmkHHHHmHHkmHmkmHmkcHmHcHcHmHcHmHcHmHcHcHcHcmcmtmcmtccmcmcmcmtmcctmcctctmcmcmtmcmcctmcctccmtmc
9OOO9D9O9O9O9OGOG9G9G9GOG99O9O9O9OG9GOOO9O9O9O99G99O9O9OOD999Z9OG96GOO99e6egBG699ZGO0qtHUMM#89jk2HwUHLUwVyUXLrHmcUkHkwhTUkHwHUjcHHjC0rHOcje0q06O6OeOeZC9qUZ#CCOcUq#Hcv      ,:;vUVvh;                        H@mmHmHmkmkHkHkmHmHmHmHmkmkmHcHcHcHcmcmcmcHcmcHmHtmcmcmtmtccctmcmtmcmcmcccccmcmtmtmcmcmcmcmcctmcccmcmtc
OOD9O9O9OOOGO9999GOGOG9GO696GgG6D99699O9O9O9O9OG9GO9O696OOO9GgO9OG96GgOOZO#ODGO9#eeDpb0kw4b8jUHbcrrHcMrrYMVrNpHU4mHkUkhXXUk4UcC#4jtt2TqOtOO@#@00OeOOZ0eek#BZ0CUAteChw,      ;lrkjtAy                        rOtcHHHmkmHmHmkmHmkmHmHmkmHcHmHcHcHcmcHcHcHcmcHcHcmcmcmcmtccmcmcmcmtmtmcmcmcmcHtccctccccmcmcmcmcmcccctmt
OOOO9DOO9OOO99G9G9GOGOGO999O9O9OOOg9G99DOD9O9O99G9G96OG99O9O99699O9O9O6O6GODG9GOGGD2qjjUH02HmCHcMj0mkrTvrUcmpXrwmUkmkwNhkkcUHmZ@cqCm9c8#tODCCjOD6O9G@ZeOGOZD#jCU#CHmAv    ..rNc0bHA,                       Y@cmkHHHHHHmHmkmHmkmkcHmkcHmHmHcmcmcHcmcHmHcHmmmmcHcccmcmcmtmcmcmtmcctmtmcmcmcmcHcccmtmtmcmtccmcmcccctmcc
ZODOOOD9O9O9O99G999GOGO99GOODGOOO9OO96O9OGOGOGOG9GO69G9G99OOOG9G99DOOGO9DOOGGe6gOZC99bmCc@UHm@kHXqcwvvlACpUTrAyHkkkN44wHmmHHHj0qt#q2q0ZD0ZC#OOO99B9gGebCZ#GBpZ2cpkp2Av   ,;vrwjC4wr                       :9CcmHHHHmkHHmHmHmHmHHHmHmHmkmHmkmHcHcHmHmmmmmmmHmHcHmHmmcmcHcmcHcmcmcmcmcHcctctmtmcmtmcmcmcctmcmcmcmtccmc
OZODOD9O9O9OGO9O69G969GO9O6G696O9OOO99G9G9G99OG9G96OG9G9OD99GGGOGOG9G9G9G96GGG9ZCtqOObqtNmZmtHjH24NllTUCmhryUMXUckHHUrNcpUUUj0Zjqtjb0CcqZjDOBg9OgeG9eZ2tAyhm0qZCchqyrv  .;vlVypmhAL                      .tZjUHHHHmHmkmHmkmHHkmHmkmkmHcHmkmHmHmHmHcHmHcHcHmmcHcmcHcHcHmHcHcHcHmHcHcmcHmHcHmHtmcmcmtmtmcmcmcmtmcmtmtm
ZOD9OOOGO9O9OGOOOG9G9G9G9996OG9GOGO9O99G9GO6O99G9G9G99OGOODG96OG9G969696GGG699OOq6eO@e@r4b#pcmpXHAyAA4UATwTkNNNcmhMc4Um2jchkjZ0p@2jOOZqDZOOe6ObOGeDeqbD0m0mHkkNkrrHkVv   ,;v;AHlywv                      cqtmHkmHmHmHHHmHmHmkmkmkmkcHmkcHmkcHmHmHmHcHmHmmcHcmcmcHcHmHcmcHmHcmcmcHcHcHcHcHcHcmcHcmcccmcmtmcmcmcctmcmc
OZ9O9O9OOD9OG9OO9O996OGO9OG99O6O9O699ZGOGO999O99G9GGG9G9gGOZOO69g9G9G9G9g99OOZ99e9O0#0Cyt#q#XrCvHjUk4vNNUAATUXkUHUHkmkmmmHcHjCqtqpCCZ0GZOOZ96ZDGB9D9C26bCOqkcXyrMVwwvY, ,.vvvwHvLv.                     UC2HUHmkmkHkmkmkmHHkHkmkmkmkmkmkmkmkmHmHcHmHcmcmcmcmcHmHmmcHmHcHcmcHcHcHcmcHcmmHcHcmcHcmcHtccmcmcmtccctmcccm
ZOZOD9D9OOO9O9O9OG9GOGOG99OGOG99O6G6999GOGOGO9OGOG9GOG9G9GOODG9G96OGGg9GOG99OGO9Z9O2t0CZpj2qyAGZtmTLVyrAVLrwANMMwUTkkmHXhmcpqOqCjbbjjbqtmq@9@Z#9OO9gDZCpmmArwyywAAyyvA; ..,yyUArvv                     lqjcmkHkmkmHmkmHmHmHmHckcHmkmHmkmHmHmHmHcHmHcHcHckmHmHcmcHmHcHcHcHmmcmcmcHcmcHmHcmmHmHcmmmcmcHtmcmcmcmtmtmcmc
D#OOOO9DGO9O9OGO6OG9GOGO69GOGO9OG9GOGGGOGOGO69G9G9696G6G69GGGO69GOG969GOG99O6OOD9O9cbDbUH4cC2yAVv;;;vlyvvvwwrThAkNXcUUHT4c00DqCtbHtH0qcmCCqmqC2Cqp@CCcHHUAYvryVlVYVyAr;.;::::v;,,                     vCCHcUHHmHmHmkmkHHmHmHmkmHmkcHmkcHmHmHmkmHmHcHcHcHckmHcHcHcmcmmHcHmHcmcmcHcHmHcHcHmHcmcmcHcHcHcmcHcmcccmcmcmtm
DOO9DOO9OGO9O9O9OGOG9G969G9G9G9G9GO69G9G9G99OGOG9G9G969GGG9G9G9G96OGO9O696DGOZ0O9GjUDZtNYrVLLv;v:,.,;vvYvAMNwXhTw4ATwkUkktctmmHcXNHkUHAXHwUNHHXmcHmcHkUUwrvvVVlv;;;;lv  .;;. v,                      .CDmmHHHmHHkmHmHmkmHmHmHmkmHmHmHmHmkcHckmkmHmHmHmHmkcHmHmHcHmHcHmHcHmmcHmmcHcHcmcHmHcHcmcHcHcmcHtmcHtmcmcmtmtmt
OZ9DOD9O9O9O9O9OG9G969G9GGGOG999G9G9GG9OGOGOG96969G96969eGg9GO6969G9G96O9O9ge@qO6qqj#mcAvrr;:;;,;;THqC2tcpbmtHjmpqCjtccHHUwAwMwTYvrlyvyALvHjyAm4XXHkmUkUXhLVMVYvv;;:.        ;                       UqmmkmHmkHHmHmHHHHHmHmHcHcHmkmHmkmHmHmHmkmHmHmHmHmkmmmHcHcHcmmHcHcHcHcmcHmmmmcHcHcHcHcHcHcHcHcmmmckcmcmcccmtccm
ZODOD9OOOOO9O99GOGOG969G969G9G9GOG9G9G96OG9G9G9g96GGGg969G6eGG9G969G9eGGO9O6O#@e#CC92tV4rvVv;;,vUeB8B8B888B8B8B8B8B8B8BegGOG9GO9OOZZ#ZDOZ9ODjbctHtkUkmk4whUcXryAVyVv,:;.                            yOtHkmHmHHkmkmkHHmHHHmHmkmHmHmkcHmHmHcHmHckmkmkmkmHmkmkcmmmcHcmcmcmcmcHcHcHmmmHcHcmcmcmmHcHcHmkmkmHcmtmcctmtmtmc
OZOZOD9O9OGO99GOG9GDGOG9G9G9G9GOGO6OG9G9G9G9G9G96Gg9gGg969GG69g96G69G9G9GGeDZDG9btcccpwAV;;,,:vrjD8BB88B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8B8e8B8BBeBG90600GgCZqtUCmNUMlrvAv;;  ;                      ;CjckHkHHmUHkmkmHmkHHmHHHmHmkmkmHmkmHmHmHmHckmkmHmkmHmHmHcHcHmHcHcHcHcHmHcHcHmHcmmHcHcHcHcHcHcmmHcmcmcmcmtctmcmtm
OOOOO9O9O9O9OGOG999G9G96OG9G9GO6OG969G969GGgOG9G96GgG69gg6O99eG6G69G9G969GD9ODDeOcUqDUTV;;.  vhUkZgegeeBBBeee8B8B8e8eBBBeBeBeBeB6geeg8BBZ#pO@8Zq9BGOe#t8#pGZjmU92tj#mACX;..T. . ;vvv;:             cqcHHkHkHkHHmHHkmHmkmHHHmHHHmHmHmHmHmkmkmHcHmHcHmkmHcHmHcmcHcmcHmHmHmHcHcmmmcHmHmHcHcmcHcHcHcHmkmHtmcmcmtmtmcmtmc
9Z9O9OOO9O9OG9GOGOG9G9GOG9GOG96969GOG9GGG9G9G969GG6GgG6GG96GG96G6969696969OO9O9GZjCUXXhrhMVvvVUANmZZeBeO9ZOZ96BBBeBBBB8eBBBeBeBB8B8eBB8G#Z@#pG9bqBqZDZjOBZ8p0#cZ0Z8tw0CVwLXjAUc4#btmk;  ::...     U#cHcUHHHkHHmHHHmkHkmHmHmkmHmkHkmkmkHkmHHHckmkcHmHmHckmHcHmmcHcmcmcHcmcmcHcmcHcmcHmHcHmmcmmHmHcmmHmmcmcHtctccmcmtc
96O9O9O9O9O9OOOGOGO996OG969G9G9G9G9G9696969G9g969gG69g9g96G69GGeGe969G99O9GO#G9CbZtXr2kXUkMHHkkHcbj@##0@ZOZOO99eGeeBeBeBe8B8eBeBe8BB966ZC@D#COBB2emGCU#CBjCpttmCOg6UYe9kBBVekCtvrA;AUbyvvAv;:.   lCp4UkmkmkmkHkmkHkmkHkmkmHmkmHmkHkmHmHmHmkmHmkHHmHmHmHmHmHmkHHmHckmkmHmkmHmHmHmHmHmHcHmHmHmHmHmHcHmHmmcmcmcmtmtmcct
eGGO9O99OO9O9OG9GOGOG96OG96O6OG9GG6969GOG9GG6Gg9gGgOGG6GG9gG6G6GeGg96GG99Dg99#CcZjHpcbmw4MrAyhpUt#9GO#O9e6eeGO6Ge6B6e6BeBBBeBeBge9GZZ@#0#bjObrDB2cZC8H8B2DbUkyv;CHGllpGLUj;j;.llkyvvvlkrvvv;;,  vCCkkUHHHHmHmkmkHkHkmkmkmUmHHHmHmHHkmHmkmHmkmkHHmkmkmHmHmHmHmkmHmHmHmHmkcHcHmHmHcHcHcmmHmkmHmHcHcmcmmHcHcmcmcmcmcmcm
9gGG969GO9OGOGOG9G9996OG9G969GOGG69GGg9G96GG9g96966eGg9g9eGeGe9G96969g9G9G99CbCOGZHHjO4UXkU4Hkctq9OOZOe8BBeBeeGe6e6eGe6ege6e6e6eeOCZ00CZ#CTD9Ym8tkBc8;t8vZGtcNlMh:#:vmqV;VvNU,;cbwALy;vMrv;:   v2pmmkHHmkmkmkHkmkHHmkmkmHmHmkmHmkmkHkmHmkmHmHmHmkHkmHmkmHckmkckmHckmkmHmHmkmHmHmHmHcHmHmkmHmHmkmkcHcHmHmmcmcmcmcmcmt
ege6gGG9OO99G9GOG9GOG9GO9OG9GOG96GgG6GGG69gG69gG69eeeGG9e6e6eGgG6GgGgG6GG96qCOgO6bHHZ2tC@j2kmH0bZC@bqZGD9GeeeGBgeGG9GGeGe6e6eGe6O20@ZjOO@@wb8Vb8hvevO;wBVvOhYVtOy ZNLk#V;Vl,@,YXV;rvHrv;;;,   ;C#kmkHkmHHkmkmkHkHkHkHkHkHkmHmHmHcHmHmHmHmkckmkHHmHmHmHmHmHmHmkmkmHmHmkmkmHmHHkHkmHmHmHckmkmHmkmHmHcmmHcHcmcHcmcmcmtc
G6Gg9e969gG69G9G9GO99G9G9G9GGg9696Gg9G96G696Gg966eGeGeGeGe6e6eGeGgGg969G9GD9GgZbqZ##GZcO2Cmcbj@bbDCcjCpttBGGeeGe6eg6G6969G9GGe6eGe#Be#GGpZrkjy,8yve;V; 8. GM;lAV; CvvmY;.;.,j :,.v;;UY;;;:   :c#HmkHUkUHkcHmkcHmkHkHkmkmHmkHHmkHkHkmkHHHkHkHkmkmkmkmkmHmHmkmHmkmHmkmHmHmkmHmHmHmHmHmHmHmHmHcHcHmHckmkcHmkmHcHcmcmcmc
69G96GG96969G9GOG9GOGO696G69g9696969G9g9g969gGeGeGeGgGg6eGegeGe6G96669g9G99O6ODCZeejbt#qHZ0cjU022bjHOqbpZeOOe9GGeGgG6Gg9GOeG69geGGq6G#ettHYpvY,jjlp    8, Zl ;vl .g.4v   . ;; ; .. :r..:;.  ,cCkmUkUkUHUmHHkmkmkHHmkHkHkmkmkmkmkHkHkmkHkmkHkmHHkmHmHmkHkmHcHmHmHmkmHmHmkcHcHmkmHmHmHmHmHmkcHmkmHmkcHcHcHmkcmcHcmcmcm
66GGGGGgGg9G9GO6O6969G9g969G969GGg969G969gGGgeGe6eGeGe6eGeGeGeGe96Gg6eG696G9O9ZOpHOeppZCXOCmccq#jjjCD0cpC#jO96G696OOO996O9#O9e99OZbOZ9ZUtyvO HXkDvA.v.l8..2v  ,, ;l c,   v    l;   ;; .::  .U#HcHUkmUkUHHHkmkHkmkHHHkmHHkmkmkHkmkHkmkHkmkmkmHmHmkmkmHmHmHcHmHmHmHcHmkmHmkmkmHmHmkmHcHmkcHckmHmHmHmHmHmHckmHtmcctmtmc
e669gGGGeGg969G9G969G969gGG9G9g9eGeGeGgG6G6Ge6ege6eGe66GG9GGe66969eGe669GGGOODO9eHOB2mc0j#CCOtqmHcCkpjb2b#0btZeG6OOZOOGOGOO@Zq99GG#p9G@cCvUj OyvGrhrt wB :C::.,,; ; m  .,M  . ,    .  ,.. ,kqmmUmUHUHUHUHkHkHkHkmkHkHkHkmkHkHkHkHkHkmkHkHkHkmHmkHHmkmHHHmHmHmkmHmHmkmkmHmHmkmHmHmHmHmHmHmkmHmkmkmHmHcHcHmHcmtmtmtmtm
Ge6eG696G6G69g96969g9G96G69G9GG6GG9G9GGeGg9gGgGe6eGeGe6eGeGeGeGe6696G69G9g9GOOO99OOZZZCOO90bOO2cbjX2pjjbHtccmtDG9GOZDGZD9gGOhm0@O89OGZC0mvZj.2Y;bly;V r8  U.v v,; ;Yy ..U. ;.        ,    rZtHkkUmkHkHUHkkkHUHkmkmHmkmHHkHkmkHkmkmkmkmkHkmkmHmkmkmkmkmHmkmkmkmHmHmkHHmHmkmkmHmkHkmkmkmHHHmHHkmHmHmHcHmHcHmHcHHmcmcmc
gGgGgGe9696G6G6G69GG6G69gGg9G9G96G69G96GeG6G6GeGe6e6e6e6eGeGe6eee6G969G96GG969GO996O9OGGB9DZ8DtmbkmjtmtmcHmmbtZ@Z9gZOC020OO6CcUr2Bg09q0ChVeN eH HAyvM MC  y;. ;  . U  ,.k  ;            .wDkkUHkkUHUHkkUHUHUkUmHmHmHmkmHmkmkHkHkmkHkmkmkmkHkmHmHmkmkHHmkmkmkmHmkmkmkmkmkmHmkmHmHmHHHmHmkmkmkcHmHmHmHcHmHcHmHcHcmcmcm
6e6eGe6g96G69G969gGG9G96G6G69G9GGgG6GgGe96G6Gg6e6eGe6e6e6e6eGeGeGg6e96GG9699OG966eO99gG#2OZbqjUpcmbcHpmH#qjCbpC2jZOGqjctHjHCDgcwAjbpCjwmAXA,,9V:Xcvyv hX vv;  :: .   v .. :.           :U0tkUmkHkHUkkHUkkHkkkHkmHmHckmkHHHHHkmkmHHkHkmkmHmHHHmHmHmkmkmkmkmkmHmkmkmkHkmkmkmkmkmkHkmHmHmkmkmkmHcHmHmkcHcHcHmHcHmmcmcmt
ege6eGeG6GG96Gg6eGgG69GG69eGGOGOgGG969gG6G6GeGgGe6e6e6e6egegeGe6ege6eGGGeGG969gGGGgGg99#@O90bC@ZcCZcqbHkqjc20mjpCm2Opj2ctkUNNmmOZArT4UkMXhYYHv;yVvlvv;AYvYl;v;::,: .vv    v           vtZmcUHkHUHkHkHkHkkUkUHkHUmkmkHkHkmkHHHkmHHHmHmkmkmHHkckmHmkmHmHmHmHmHckmHmHmHmHckckHHmkmkmkmHmkmHmHmHmHmkmHmkmHcHcHmmmHcmcmcm
eegege6e6e96Ge9eGgGe9GGG96GGGG999G9egGG6Gg9G9ege6e9eGeGBge6e6geeGeGe6eG6GGG69G9e999699OGGg96cm#OtGCcjmcXpHUO@jqqUhmbH@@mTXNNTMAHchyrvYVrrhAUVrMrlvyVLwrhwrVrlyvyAyYVv;;;;v;:         v#CkkkkUkkkUkkkUkUHkHkHUHkHUHUmkHUHUHkHkmkHkHkmkmkmkHkmHmkmkmkHkHHHkmkmkmkmkmkHkmkHkHkHHmkHHmHmHmkmHmkmHmHmHmHmHmHmkmmtmtHcHmHm
egeGegeGe969GG69gGegG96969G9g969GOGGeG6GG96966ege9eGegege6eGgGege6eGeGeG6G6Ge9g9gGG96Gg6Be9D6ZO@bOpcZ2jmZ@btUcccH4kMAH@j4VAXXrMmAYhUAlyVYvlvlllVyVyyryrrhLhTThHcUyrlwXUAv;;;;;v;.   rCtkHkHUkUHUkUHkHkHkHUHUHUHkHUHUHUHUkkkkHkHkHkHkHHHkmkHkHHmkHkmkHHmkmkHkHHHkmkHkmkmHHkcHmkmkmkmkmHmkmHHkmkmkmHmHmHmHcHcmmmcmmHmH
geeBeegeG69GOeO69696G6GGG6Gg9G969G9G9gGg96G69e6e666e6e6e6eGeGeGege6e6e66G69egeGGGgGgGg9G9GZZDZqODeOZCjkqC#mjtpHcXhwMvyXHMYlhyVvyyyvYYYvlvvvvvYvvvvvYvYlVllVLVrVVvvLyyVvvvv;vv;.    U#jkHkkUkUkUHUkkHUHUHUHUHUHUHkHUkUHUkUHkmkHkmkHHHkHkmkHHHkHkmkmkHkHHmkmHHkHkHkHkmkmkmHHHmHmHmkmHmkmkmHmHmHmkmHmHmHmHmHmkmkmHmHcmc
eeBeeeeee9G9696O69G9e6e6gGeg6Gg9g9GDG9gGgG6G66gGe9ege6ege6e6eGeGeGeeeGeGg966Bee9G9GG69G9GOODOO6ge#ODmj@2j0jpjUtcUAXrXTAlYyMrvvvvlv;;v;vvv;v;vvvvvvlvlvlvVvVvvvv;;;v;,,,,,,.,.     UZcmHUkUHUHkHkkUmUHkHkkkkUkkkUkUHkHUHkHUHUHUmkmkmkHHmkmkHkHkmkmkmkmkHkHkHkHHHkHkmHHkmkHkHHHkmHmHmkHkmHmkmHmHmkmHmHmHmHmkmHmHmHcmmH
eBgeeBg8gG99O6OGGG9g6eGeGg6e96999G969G969g9eGe6eGgGgGeGg6eeegegegeGeeeGeGege6e969e6BGGOOZ9GBGGGBGZCOptDmk@22qUHjHmmy4XXrvYAhlv;vY;;lvv;v;v;v;v;vvvvvvvvvvvvv:;,:,,....           yOtUmUHUHUkkHUkUkkHUHkHUkUHUHkHkHUHUHUHUHUHUHkHkHkHkHHHkmkmkHkHkHHmkHkHkmkmHmHmHmHmHmkmHmkHkHkmkHkmkHHmkmHmkmHmkmkmkmHmHmHmHcmmmcHc
eeeeegBge9699969G9696G6GgG696G6Gg9G9GOG969GGe6eGgG6GeGe6e6e6eeegeee6e6e6eGe6eGgGeGeeeG9Og6g99OeeeOOqO@ZCCOOpOmjqpUHUmNrlyylyVllv;;L;;;;;;;;::;;;;:;;;;;;;::,,.,.. .              cjHmUmUkUHUkUkUkUHUHUHkHkkUHUHUHkHUHUHUHkkUHkHUHkHUHUHHHkHkHHmkHkHHHkHkHHHHHkmkHHmkHkmHHHmkmkmHHkmkHkmHmkmHmHmHHHmkmHmHmHcHckcmcmcm
Gegege6e6GOGO69G96G69G9g9g969g6BG6GG9G9G9G9GGgGeG6GeGgGe6e6egegBgBgegeeegege6egg6eGGGe9eeBG99gOO@O#6Z9BZXeO2cMtcrcHkjMvANrvrvlY;:hA,.;lv.,.,........     . ......               VDkkUkUHUHUHUHUkUkkHUHkHUHUHUHkHUHUHUHkHkHkHUHkHUkkHUHUHUHkkHmHmkmkHHHHmkmkmkHkHkHkmkmkmkHkHHHkmkHkmkmHmkmkmkHkmkmkcHmHmkHHmHmHcHcHc
ege6ege6e9GOGG9OGOG99OGGgGg9eGg9eG696OGOG9G96G69gG66eGeGeGeGe6e6egegeeege6egege6e6e9ggeee9GOGGg9GBB@bBepm94cCwejUZchwywTyVLyvvAvVl;.Y:v: ,v;:,:,,..                            :qjH4U4HUkkHUkUkUHUHkHkHkHkHkHkmUHUHUmkHkHkmkHkHkHUHUHkHkHUHkHkmkHkHkHkHHmHmkmkkUHUHkHkmkHkmHmkmkHkHkmkcHmkmkmkmHmkmHmHmkHkmHcHmHcHcm
6e6egeGBGe969G999GOGOG9G9GGg9g999g9eGeOG9GO69G96G69gge6eG66egeeegege6e6egege6e6egBeege6e6eGeZZG6geOjC6UCO6t#9tm2tkrXAUVrATVlYNrNvwl;Vv;:.vv:,;:;.,..   . .                     HbHUk4HXk4HUkUkUkUkUkUHUkkHUkkHUkUkUkUkUHkkkkUkkkUHUHkHkHUHUHkmkHHmkmHmHmHmHmkmkHkHUHUkkHkkUHkHkHkmkHkmkHkmkmkmHmkmkmHcmmkHHmkcmcHcmc
e6eGegegeG699OG9G9G9G9GOGG69GGg9696GgG696OGOG9G9gGgGe6e6gGegegege6ege6e6eee6ege6e6e6eGeGgGBBDZBeB9eZDpMOZqtmcMUBtYUyMkyrryyhyYYvvH;;y.:;v,..... .:. .   ..                    ;ZpUkUkUUUkUHUkUHUHUkUHUHUHUkUHUkUkUHUkUHUHUHUkUkUHkHkkkHkHkHUHkHkHkHUHUHkHUHkHkHkHkHkkUHkHkHkHUmkmkHkmkmkmkHkmkmHmHmkmkcHmHmHcHcmcmcH
Ge6e6ege6eGGOO99OGOGO9OG9GG69G96G696G6Gg9G9GOGOg9g96G6Ge96Gegegeeegege6eeegeee6eeege6eeeeG9G9GegG9ZGODqOb0mC4CN9cTHw4XXT;Ylyl,YV,v;v:;;;, ,...  ,v                            yCkk4kUHUkUHUHUkUkUHUkkHUHUHUHUkUHkkkHUkkkUkUkUHkkkHUHkHkHUHkHkHUHkHkHkHkHkmkHkHkHkHUHUHUHUHUHUHkHkmkmkmkmkmkHHmHmHmHmkmHcHHkmHmmmmcmm
eeeGe6e6g6gOGOG9GOG9GO9OGO6G69666G6G6G69gG69G9G9G9696GgG69gge6egeeeeBge6e6egegege6egeg6Oe6eGOD6OgGcD9UOqDbcptBAhCkbyvwUv;LAlV;v,;vl;v,.,:;v..   ,;                            tckUkUkUkUkUkkkUHUkkHUkUHUHUHUHUHUHUHUHkkkkUHkHUkUHkHkHkHUHkHUHUkUHkHkHkHkHUHUHUkkHkHkHUHUHkHkmUHkHkHkHkHkHkmHmHmkHHmkmkmHmHckmHcmcmmm
Ge9eGe6e6G9gGg9G9GOOO69GOG96GgG69696G6GG9e6ee69G96969G96Ge6g6eeBeeeBeBeegegege6e6egegeGe6eeeGg98e#cO0c6Z00q#G0VhjXUyvHHAvyYv;yv;;v;;,;, ,: . ..  :                           v@HUHUkUkUkUk4kUkUk4kUkUkUkUHUHUHUkUkUkUkUkUkkHkmkHkHUkUkkHUHUHUHkHUHkHkHUkUkUkkkUHkHUkkHUHkH4kkmHcHcHmHckmkmkmHHkmkmkmkmHmkmHcHmHmHmHc
B66GgGg6eGeGeGg969GOGOG9G9696GeGgG6GgG6Gg96GBGG96969G969e9GG66eee6e6egegegBeeeeeeee6eGggBee9GGeeBqCBBj9jO0b4rrD0@wmCMhH;vyX;:Av;vvv;.,.:; .;: ,. :                           UZNkUk4HUHUkUkUkUkUk4kUkUkUkUHUk4kUkUUUkUHUHUkUkkmkHUkUHkkUkUHUHkkUHkHkHkHkkUkUkkHUHUHUHkHkHUkUHkmkckmkcHHkHkHkckHHmkHkmkHHmHmHmHmkmmcm
B8eeeeGeGgGe6e9gG69GOG9GOGOG9G9G96G6Gg6e96969e6e9GO69G9G96GeG6Ge6eeeee6e6e6egeeeee6eGeGeeBeee89O9D0B0jp9OctkvtBHwMMc;y4vvUA;vUvvTv,:.:.;;;;:.    .                          .OjUUUUkUUUkUkUkUk4k4kUkUk4U4kUkUk4k4UUkUkUkUkUHUHUHUHUHUkUHUkUHkkkHUHkHkkUHkHkHkmUHUHUHUkkHkHUkUHkkkmHmkmHHkHkmHmHmkmkmHmkmHmHmHmkmHcHc
Be8B8BBeeGg9eGeGg9G999GOO99O99GO6GeG6GeG69eG6Gegg9GOGOG9GO6GgG69e6eGe6e6eeege6eeBeBeegeeBge9G69ZeeBO#egB#cGUl8pYjUvlMkUvvkvvNL;Vy;;,;,v;,.. .                               lCU4UXkUUUkUkUHUkUHUkUk4UUkUU4kUk4kUkUk4kUkUkUHUkUkkHUkUHUHUkUHUHkkUHUHkHkmUHUHkHUHUkUkUHkHUHkkUkUk4HkHUHkmkHkHkmHHkmHmkmkmkmkHHmkcHmmcc
6e966BeBB8eBBB9G9G9G9G9G99OOO9O9OGO69g9G9gGe9G9g9666Gg9G999696G6GeGeGgGe6e6eee6ee8eBeeeB6g9GOGGeGBe9OeeCkhtCLGtZtMhjmyrrrwUMyvvy;wv;v;;;:;:,       .                        4bhH4U4UXU4UUkXkXUNUXUXUNUXUXkXUXUNU4k4U4UUUUkUkUk4kUHUkUHkHkmkHkHkmUHUHUHUkkkUHUkUkUkUkUkUHUHkHkHkHUHkHkHkmkmkHkmkmHmkHkmHmHcHmHcHmHtmc
B6e6gGeGegBe8BBeeG6O9OGOGOGOGOG9GOG969G969g9gGG9g9eG696O6OG969g9696Gg6e6eeeeeee6BeB6e6eGe6e6BeB6gGGbZBDwH9C0ZqjHrUttwUHyyAjY:VM;vvv,:;;:,          .                       .pthkHXUXkXUXkXU4kXUUUXUUUXkXUXU4UUUXUXUXkXk4k4kUkUk4kUHUHUHUHUkkHUkUHUHUHUkUkUkUkUHUHUkUkkHUHUHUHkHUHkHUHkHkHkmkmHmHHHmHmHmHckmHcmckcmcm
eeeB6eGeOGG6G6eBBBB8669G9GOG9G969GOG9G96969G9gGG96G69G9G96969GGg9GG696GgGe6eeege6ege6BB8eeGeGeG66gOG0e2OCe22cMZmk#cjMwwMLkv;VAvA;;V;.;.,                                   AjcwkUU4U4U4U4UXkXUXUXk4U4UUUXUUk4k4UXUXkXU4kUkUkUk4HUkUkUkkkUkUkUkUkUHUkUkUHUHkkkHUHkkUHUkUkUHUHUHkHUHUHkHkmHmkmHmHmHmHmHmHckmHcmckmHcmc
BgBeBeBeegeGeGgGgGee8BBeg9GO9O999O9O9OG9gGG9G9G9gGg9696G69GG69G9G9696Ge6e6e6e6egeGeeBeeeBeBeeGe9O#e9#OUj8BCcmtGcXXLc4NkvwA;yv:rv;lv::,  .                                  ttUUXU4kUUUUXUXUXkXUUU4UUkUU4UXUXkXUXUXU4kXkUk4kUkUkUkkkkkkkUHUHUHUHUkUHkHkkkHUHUkUHUHUkUkUHUkkHkHkHUkkHkHkmkmHmkmHmkmkmHmkmHmHmHmmmHmHcm
eBeBeBeB6eee6eGeGgGGGegee8ee6GO9OOZOO69G9G9GG6G696G6Gg96GgGG9G9G96OG96gege6e6e6e6g6e6e6eeB6gGeeBOO9eebH66C0kwjXmbU2tlw4yATlk;YHvVv;.   .          .                       l0mUkUXUXUXUUU4UXUXU4U4kUkUkUUXk4kXkXkUUXUUUXUUkUkUkUkUkUkUk4k4kUkUkUkUHUkUk4kkkUHkHUkkkkkUkUHkHkkkHUkkkUHkHkmkHHmkmkmHmHmHmHmkmHmkcHcmcHH
BBegBGe6egeeegeGegeGe6e6e6BeBeBe699OGOODGOG9eGgG6969GG6G69G9G9G9GG696GgGeGeGege6e6e6e6egegeGBg69GGBGCZ8DcZ@c#t#CCAHrlkcvNYVwvyL:..;  ....         v                       cCUXHwU4UXkXUUUUk4UUk4UXkXkUUXUXk4U4UXUXU4U4kUkUkUkUkUkUkUkUkUkUkUH4kUkUkUkUk4kUkUHkHUHUHUkUkUkUHkHUHkHUHUHUHUmHcHmkmkHkmkmHmHHHmkmkcHcHHk
B8eBeBeBeBeBeB6Bee6eGeGg96O9O6geB8BBe6GG99OOOG9696G6GG96GgG6969G9G9G9G9GG66e6e6e6e6G96Ogge6ege9GGee#j@gbZZ4qO0jvMHbjMhHvyMvhhV;,,;,    :         :M                      ;2mUXUHXkXU4kXk4UUUXUXk4kXUXUXU4k4k4kXU4kXUUkXkUkUHUkUkUkUkUkUkUkUkUkUkUHUHUkUkUkUHUmUkUHkHkkUHUkHmkHkHkHkHkHUHHmHmHHkHHmkmHmHcHmHmkcHcHcHm
BB8B8e8BBeBeBeege6ege6g6e669G99O99eeBeBBBG9ZOZ99G96GgGg9G969G9G9GOGOGOG9gGe6e6gGe6eGG9G96GeG66eGgGO#ZCG@Ojj6j4hYUbGclrcVkVlky ;;; .;. .          ;h                      U2XHT4kUNU4U4UXU4UXU4U4UNUXU4kNUXUXUXkXUXk4UUkUkUHUHUk4kUk4kUkUkUHUkUkUkUkUkUkUkUkUHUkUHkkUkUkUkUHkHkHUHkHkkkHkmkmHmHmHmkmkHkmHmkmHmHckmmcm
B8B8BBBBBBeegeGegege6ege6e6eGg9G9GOGOG9geBB8eBGGO99GO696G6GeGeOGO69G9eG696G6G66egeGe9GGe9GO996gg9999OG9OOjp9klDZMDZv;XNVvlmv,:l.;;   .           yA                     ;qm4XUXUNUNUhUNUNUNUXUNUh4hUXUNUNUXUXU4U4U4U4kUkUUUkUk4k4U4kUkUk4k4kUkUkUUUkUkUk4kUkUkkHkHkHUHkHkHUHUkUkUHkHkHkHkmkmHmHmHmHmkHkmkmkmHmHmmcmc
eGg6eeBe8BBB8B8BBeBeBeBeBee6egege6eG699O9969eeBe8eBeeGG9e6g969GOGO9OgGg9gGG969g96GeGGOeGg9G9GOG966G69O9jqtZ@UMtDp0kAUwUrlrM lyv,;;, .:           cc.                    mjkNUXUNUX4XUX4XUX4N4NUX4N4X4XUXUhUX4XkXU4k4H4U4kXkUk4kUU4k4kUk4kUk4U4UUkUk4kUUUkUkUHUHkkUkUHUHUHUHUkkHUHkHUkUHUHkHkHkmkmkmkmkmkmHmkmHmHmmcH
GGOG9e6e6ee8BBB8eBeBBBeBeBgege6e6eGeGe969GO9O99GGgB8eBg666O9ZO9GO9O9OOZO9e66G69G96eBgGge96O6O9DOO6OZgBjHm2qCHmtBUmyNtXXUv;vlr;; ,. ,             GM:                   ;jcXUNUXUXUXUhUNUXUXUhUN4XUNUX4X4NUXUXUXUXUXU4U4k4UUkXU4UXk4kXU4UXkUUXUXk4UXUUUNUUHUkUkUkUHUkUHkHUHUkUkUkUHkHUHUHUHkmHmHmkmHmkmHmkmkmHmkmHmkc
eeGOG99OGOG6egBeBBBeBeeGe6egege6e6eGe6GGG966g9GO9ZOZG9e6BeBeee6OGDO#9Zqb0C9GeBe9GGe6eGeGe9GO6OOO9OG9ZZDttCptqtqmHkXyAhUv;;yY;,. :  v            Yel..                  NjkkhUX4NUNUNUhUNUXUXUXUXUXUNUN4NUXUXUhkXUXUXkXU4U4U4UUUXU4kXk4UXkUkXUXUUU4UXU4U4kUHUkUkUHUkUHkHUHkkUHkHkmkmUmUkkHUHkmHmHmkmkmHmHmHmkmkmHmHmH
6e669696969G96Ge6eeBeBeBeB6egeeeeeee6eGe6e6eG6GG9G9G99O9OG966BeBeB9G#Cb0jtjbCGB8gg969e96G69GGOqg6D@GZO9OGjpGpck4UkNATMrvvlvv.v,l, ;,           vOjv                   rbHHhkXUXkNUXUXUNUXUXUNUNUXUXUX4XUh4NUXUUkXUXk4U4kXkXkXkUUUU4UUk4UXkXU4kUkUU4UXkXU4UUkUkUkUkUkUkUHkHUHUkUHUkUkUHkHUHUHUmkmHmHmkmkmkmkmkmHmHmkm
e6e6eGg669G9G9G9g6g6gGe6eeBeeBBBeeeggGeeegege66GGG696O69OOODOZODGGB6B699GZcLlycqBBBee6G9G9G9gDO96OZ99bOG9CG0NtpkkUUhUNvrL;yv ;l:.,;           v99l,                  v2HHhUh4X4hUhUXUN4hUNUXUXUhUXUXUXUNUNUXUXU4kUU4UUUXkXUXUXUXU4kXkXUUU4UXkXUXU4k4UUkXk4k4kUk4k4kUkUkUkUHUHUHUkUHUkkHUHUHUHkHkmHmkmHmHmkmHmHmHmHmH
B8eB6BeBeB6g66GG96O6969696GeGeeBeeee6e6e6e6e6eGeGgG69GGG969GO9O99GDOOOOBBB9Z2cMVAj98BeG6GG9G99OOO9DGD#D9q@b2c#0cXNwUVMrvvlyv.vv  v          ;;q6kv :.                UtUUUhUXUNUNUXUXUXUNUXUhUX4NUXUhUX4X4XUXU4kXkXUUU4kUkXkUkXkXkXkUkUU4kUUUk4UXUXUUUUUUU4kUkUkUkUkUkUHUkUkUkUkUHkHUHUHUHUHHmHckmHmHmkHkmHHHmHcHmHm
BeBeBeBeBBBeegeGeGe6e669GOG9g6egBeBee6e6e6e6eGege6ege9gGGOGOG9OO9OOD9#ZZO@GeBe92Tv;;AbZZG6e99O6OOOGOOOGCDt#ZttCtUHc4XVTlTv;vl , ;,        .vMUCtY, ,,               ApHUUXUNUhUNUXUXUX4NUNUX4hUNUNUNUNUX4X4hUhUXUXU4k4UXUXkXUXUUkXkXUXkXU4U4UUU4U4UUkXk4kXU4kUkUkUk4HUkUHUkUkUHUHUkkkkHUkUHkHHmkckHHHHHHmkmHmHmHmkmH
eBeBee6eeege6eGeGe6e66969G96Og969g969gGgGg6ege6eGe6egeeB969696OGO9O9OOD9ZD#OZOOeGqNAUOqCb#Z6O99GOOG9DG9OqCp#2ctthpHX4rlrr;;;..;,;.       .lvvCCm; .;;              vbmX4wUNU4U44X4hUXUNUXUNUN4NUXUXUXUhUNUN4h44kXUXU4kXUXUUUXU4k4UXU4kXUUUUk4UUU4k4U4UXUUUUk4k4kUkUkUkUkUkUkUkUHUHUHkHkkkHkHkHUHkmkmkmkmHmkHkmHmHmHc
BeBeBeeGeeeGe6e6g6e6eGe969G9GO9O9O9OGOeGg9g9ege6eg6966egege9eG69G9GO9O9O9DOO9OOOGGBGGOOZZCqpDD9OGO9OOOODO99bHNc4CHh4hlvyv;;. ;v.,       :l;;tqjX; ;;v.            ;mtXUhUhUXUNUNUNUNUN4h4NUhUNUNUNUXUNUXUN4hUhUXUXU4U4UXk4kXU4k4kXkUUXUXUXU4U4k4UUkUU4k4kUk4k4kUk4kUkUHUk4HUkkkkkUkUHUHkHkHkHkHUmkmkHkmkmHmHmkmkmkmH
eBeBeBeeeBeB6ege6ege6eGgGe6eG6OGDG9G9GG69G9G96GG9G96G69ggeGeGe969G9gGg99O9O9OOOOOO@ODZC@@Z0qqZCDZ9O9OD#9#qC@pjp0cXUkyVvYvv;;;;;,   ..  .rV,VqZjU;;vv,.           y2mNXNUw4NUhXw4X4h4XUNUNUXUXUXUh4XUX4XUNUh4hU4k4U4kXkXUUUXUXUXU4UXkUk4UXUXkXUXUXUUUUkUUUkUkUkUkUHUkUk4kUkUHUHkmUkUkUHkHkHUHUHkHkHHmkHkHHmHmkmkcHmHm
BeBeeeBeBeBeBeBeBee6BeBGege6eG6GG9gG6G69G9OO9O9O9O6O69gGGG69GG6Gg9G9G96OGOGO9O9O9ZDZO#DZZ0ZqZCqb@CO9eODOg@jC0kpcmcjkwTAvvvLvlvv;.;:   ,vTvYXbOCv;vv;,          ,X0tUUTXhXwXwXhXNUXUN4NUXUNUNUXUhUhUXUhUXUUU4UXU4k4k4kXkUkXkXkXkUU4UXkUk4k4UUk4UUkUUUk4UUHUk4HUkUk4H4kUkUkUHUkUHkHUkUHUmkHkkUHUmUHkmkmkmHmHmkmkmHmHmH
ggeBee6eeBeBeB6eee6egBgg6G6e6eG696G6969GOGOO9G9GO6OGOG9G96969g969G9GO99699O9OGOGOOZOZO#Dq@0@CZ0#0qbZZG96OZcj#2kmmpHNMNwrYyyvYl;;.,.  ,;vvvHDbOcvv; .:,        :tCXXUhXXUh4wUh4XUhUXUNUXUXUXUhUXUNUXUXUXU4kUU4U4k4UUUUUXU4kXUXkXU4kXkXk4UXUXkXUNU4k4k4k4k4kUkUk4HUkUk4kXk4kUHUHkkUkUHUHUHkHkHkmUHkmkmkmkHHmkmkmkmHHkm
eeBeBee6egeeeeegege6eGeGe6e6eGe6eG6GG9G9G9G9G9GOG9OO9OO99OOO9OOO9DOD9O9OGO9O9OODOO9OODODOZOZZ0Z0#q@CCCO96D2pO#CccmUM4rMX4VlVyv;:, .  v;YVwZb9Dcvvv;:;,.     ,yttmTUNUNXwUhXNUX4UUhUNUNUNUXUXUXUXUX4NUX4XkUUUU4UXkUUXk4U4UXU4kXU4UXUUU4UXkXUUU4U4kUk4U4kUkUkUHUk4H4HUkUHUkUHUkUHkHUkkkUHkHUHkHkmHHHHkHkHkmHHkmHmHHHmH
eBBBeBeB6eGeGegege6eGe6e6egege6eGe9g969G9GGg9G9696969G999G9G9G99O9OOO9D9OGOODOZOZ9#OZODOZO#Z@Z0ZqZ@ZCCC#ZeG9GZCpHcHUXwyTwrVy;v;v:.  ,lvthpqCB0MvvYvv:,.....lt2UX4UhhhUhUXUhXhUXkXUNUN4NUXUX4NUhUXUX4XUNU4UXUUkXkXk4kXUXkUk4kXkUUXk4U4UXUXUXUXUUk4HUk4k4kUk4kUkUkUkUkUHkHkHUHUHUHUHUkUkkHUHUkkmUHkmkmkmkmHmkmHmkmkHkm
BeBeBeBee6e6eee6e6e6eGeeeee6e6e6e6eGeg6G6Gee69g9G9gG696969G969GOGO9OGOODOOODOZOZD#DZOZO#DZZqZ0Z0Z0#b@qZ0C@Zq9DZqbccktUwTUwyYvvv;,   :y0OtmkCHVyvv;:.,,, ,vHjkUUNUwNNUwXhUNUXU4UXUX4NUXUN4NUhUXUXU4kNUXU4UUk4UXk4k4k4UUU4UXU4U4kUUNUXkUUXkXU4HUkUHUk4H4kUHUkUkUkUkUkUkkHUHUkUHUHkHkkUkUHkHUkkmUHUHkmkmkHkHkHkmkmHmHmH
BBBBeBeeeBeBeB6e6eGeGgge6eGegegegege6e6e6696G6G6G696OG9G96969GO9O9O9O9O9O9O9OOZOZOZOZOZOZO#ZqDq#0@@Z0Zq@C0CqCqC@CjmtH4wHUAYVlv;:  ,vAp90h4Xcvvvv:;:;;;;vAttUrXXUhUXXhXNUhX4UNUXkNUXUhUhUhUX4XkUUXU4U4U4U4k4UUUXUXkXkXUXkUk4UUUXUUUUkUH4UXUXUUkUk4kUkUkUkUkUkUkUkUkUkUHUHUkUHUHUHUkUHkHkHUHkHUHUHkHkmkmkmkHkHHmkmkHkH
BeBeBeBeBgeeBeeeeGe6G969G9GGG9G9696969e669GO6GeG6GGGG96OG9G9G9GOG9G99O9O9OOOOZOZOOODOZDZOZ#0Z@Z0#0Z0#CqC@00bCjb2pkkUHUk4Avylv;;;vlktcUAvvyrv;;v;vvrMkHjcmkkUHUUhXNXw4NUNUhUh4XUXUXUXUXUhUh4XkUkXUXUXU4UXkXUXUXU4kXUXkXUUUUk4U4kXUUkUUUHUHUkUHUkUkUkUkUkUHUHUHUHUkUHUkUkUHUHkkUkUHUHUkUmkHkHUHUHUHHmkmkmkHkHkHHHkHkHk
eBeBeBeBBBeBeBeBgeeeGeGe6e9G9G9G9GGeGg66GgGeeeGg969G96969G9GOGO6OG96OGO9OOD9OODOO9DODOZO#O@Z#D@Zq@0@CqCq0qCCpC20tXAwwXXUXUXkNUUHUcUwrVYVlvvYAhTUUHccUHUkwXXUhTwUNUhU4kNUXUXUNUNUNUXUNUNUXUNUUkUUXUXUXkXkUUUkUk4U4kUkXUXkXk4kUUXk4kUHUHUkUHkHUkUkUHUkUkUkkmkHkHkmkHkHUHUkUkkkkkkHkHUkUHkHUHkHUkkmkmkmkmkHHmkmHmkHkHHH
BeBeBeBeBeBeBBBeBeege6egB6e66GeGGG69G9G9gGee6G6GgGe969GGG96969G9GG699OGO9OOOOOOZ9O9ZOZDZO#ZqZ#Z0Z0#@#C0qqCq0@0CckAwhXhUXUUkUHUmkkkUhmkUNkkmUkUHkHUXTUhUN4NUTX4UXUXUXk4kUHUUXk4U4kXk4U4UXUXU4kUk4UUkUk4k4kUk4k4kUkUk4U4kUk4kXkUkUHUHUHUkUHUHUkUHUkUHUkkHUHkkkHUHkHUHUkUHUHkkkHkHUHkHUHUmHHkHkHHmkmkmkHkHkmkmHmkmHmHmH
GeGe6egeGegeGeGeGe6e6gGe6eG6GG96O9OODODO99OOD9OOD9O69G99OOD9D9D9OOO9DODOZ9ZDZDZZ#D@Z#ZC#q@q#0#Cq0qC0b0b0CCtpttUNrMMTAwrwAwAMAwATAAATAMATAwAMATrTAMATrAAAATAwMAAwMwwhTwMhTwMwMhMhMwAwAhAhThAhwNTXTXMXThMhTNTNwNwhwhMXwXMNwhThwNNXhXhNTNTNwXwNTNwXwXXUNUXUX4hUNUX4NUXUNUX4XUNUXUNUh4N4XUNUXUXUXUNUNUNUX4hXXUXkXUXUXUXU
*/

// A still taken from Aubree Dieckmeyer - Feeling Good - American Idol 2012 - San Diego Auditions - 2012.mp4_ASCII-aerosmith1_400.txt
/*
g6gGe6eGe6eGe6e6eGeGg6g6eGeGeGe6g6eGGOe9GOG6eGeGgGgGe6gGgG6GeGgGgGeGeGege6eGe669eGeGgGg6e6eGe6eeepkl;,.                                                                                                                                                                                                                 ,:;vH@8ee6eGe6e6eGe6eG6#trv.                         .;;vyhUcC9OgBBeeeeGgGeGg6gGeGg6e96O
6eGeGeGeGeGe6e66GeGe6eGe6e6eGeGeGgG69G96GgGgGg6e6e6e6eGg6gGe66GgGeGeGeGeGeGe9g96GeGeGeGeGgGeGgGBOpAv,                                                                                                                                                                                                                    .:vw#eegeGeGe6eGe6gGeO@ky;.                             .,;vMkpb99BeBgeGeGe6e9eGg9g969O
eGe6g6gGeGg6eGgGgGe6eGe6eGeGeGeGgGgGeGgGe6eGe6e6eGe6eGe6eGe6eGgGgGeGgGeGeGeG6G69eGgGeGeG6GeGgGgee2Xv;.                                                                                                                                                                                                                  ,:;vUqBeeGeGeGe6gGeGeGgqcrv,                              . ,:vlrAXHDB8BBegGeGeGeGg6e9G9
Gg6e6e6e6eGgggGeGegeGeGe6eGeGeGeGe6gGeGeGgG66eGeGe6gGeGe6e6e6eGe6e6e6egeGeG6GgG6GgG6GeGeGg6eG6GeeCwl;:                                                                                                                                                                                                                  .;;v40ee6gGeGeGe6gGeGeG@HMv;                                .,,;;vvYAOB8B8ee6e6g6e6eGe99
eGgGe6g6e6e6eGgGeGeGe6e6e6e6e6g6gGgGeGgGe6gG66e6e6g6eGe6eGe6eGeGe6eGgGgGeGg66GeGeGgGgGeGeGgGgGgeBCUl;,                                                                                                                                                                                                                  ,:;vUpBge6eGeGe6ege6e6Gqmrl:                                    ..;;lr4t@OeBBeBeBgB6egeG
6g6eGeGeGeGg6e6eGe6e6eGe6e6g6e6eGg6eGeGege6eGe6e6e6g6eGe6e6eGeGeGe6e6eGg6e6e6e6e6eGe6eGgGeGeGe6B9jAv;,                                                                                                                                                                                                                  .:;lA0BB6e6eGgGeGeGeggO#Uyv:                                       ,:;vVyUjDZ6B8e8BBeBeB
e6g6eGe6e6e6e6e6eGeGeGe6e6e6e6eGeGeGeGeGe6eGeGgGe6e6g6eGgGe6e6eGe6eGeGe6e6e6gGe6eGe6eGeGeGeGg6ee9mMv;.                                                                                                                                                                                                                  .,;vTD8ge6e6eeBeegeee99qXlv,                                        .,;;;vyrUtODZ0#CZ#OO
6eGe6eGe6eGe6e6g6e6e6e6gGe6gGg6e6gGeGeGeGeGe6eGgGeGe6e6e6e6eGe6eGe6eGe6eGe6eGeGe6g6g6e6eGeGeGeGBOtAv;,                                                                                             .                                                                                                                     ,:vT6B8BBB888B8B8B8B8BGUVv;                                           :,;;vvAhUNkkHkUAA
e6eGe6gGe6g6g6e6e6e6eGeGe6eGeGg6eGgGeGe6eGeGe6e6g6eGg6e6e6gGeGe6g6e6g6eGe6eGeGeGgGeGeGeGeGgGeGeeGcTv;.                                                                          ..,,;  .;;;;vvVlAMHNTVVvv;v;;;;;;;;,.                                                                                                   .,;VHO8eBGgODOGOD#OZ02qprvv;                                              ..;;vvvlyVyv;;
6gGe6e6e6e6e6e6eGgGe6eGe6eGeGgGg6e6e6e6e6eGeGeGeGeGeGe6eGe6eGeGeGeGeGeGgGeGeGeGe6eGe6eGe6e6eGgGBOjAv;,                                                             ..,;;vykCjbjb0#@GH  ;tbtpckMAAwTMAArrlvvvvvvv;;;vvv;;,::;,;::,,..                                                                                     :;VyNUUwXATLyVryVvyvlYy;,;;                                                 .,. :;;;;..
eGeGgGe6eGe6eGeGe6eGg6eGeGe6gGgGe6eGeGeGeGg6e6eGg6eGeGe6e6g6g6eGeGg6eGeGe6eGe6eGgGeGeGgGeGeGe6eBGcwv;,                                                       ..:ht##ZZDZODD0qbptjcjtc: .HHHkkXXTMYvYryryVvv;v;;;:,,.,:::,,;:, ..,.,,,.,,:.....,.                                                                         .:;;;;:;;;:;,::;,::;;;;: ,;.                                                           
Ge6e6e6gGeGeGe6e6e6e6e6eGe6e6e6g6e6e6eGeGg6eGeGeGe6e6e6e6eGeGe6e6e6eGeGeGeGe6eGe6e6eGeGgGeGe9egeOtyv:,                                                   ;lNmjtqp0bCj2jjccmmkHUHUHXUNVvvTUTTMhrwhrvYlVlVv;;vvYVLrXArYvvv;;::,,.,.,.... . . .   .                                                                          .;;: ... .     . . .     .                                                            
e6e6eGe6e6e6eGeGgGeGe6g6eGeGg6e6g6eGeGe6e6eGgGe6e6eGgGe6e6eGgGgGe6e6e6e6e6e6g6gGe6eGeGeGgGeGeGeGZHr;;.                                            .:;vhUmt2tcHHkkUmUkkkUk4HUkUkkHkkkkAAyTMMLMwAyl;vvYvvvvywXjpCq#bC2Cj220qZqZ#@0qCCbbjpjb2qCCttcA;;;,.                                                                      .                                                                                   
6e6e6eGeGeGe6e6e6e6eGeGeGeGeGe6e6e6e6e6e6eGeGe6e6e6eGgGg6e6gGeGe6eGg6e6e6e6eGgGgGeGe6e66G66gGg6gqmyv,.                                        .;v4mtm20CUwAhAAyArLyryALrAHkkkmkHHmHcHUryrMrAAUyv  :ylv;vrct2#D#O02ccctHcctcttpctmtctmcmttjtbtjmcHmHNYl;:.                                                                                                                                                       
eGeGg6eGe6eGeGe6e6e6eGe6gGeGeGe6g6eGeGeGe6e6e6e6eGe6eGeGeGe6gGgGe6eGeGe6eGeGg6eGeGe6e6eGeGgGgGBG#kL;;.                                    ,;Mmjj2HUrArAyVvYyArAAMyyllvlvlYhUkkmUmHck4AryArMLhUy:  Yyv;VcObZZZ@Z@Z0CCCtjctHcmtmmHcHtcjjpjjcjttmckUNkkcUU4XY;                                                                                                                                                     
Ge6e6g6eGg6eGeGe6eGeGeGeGg6e6g6g6eGgGeGe6eGe6g6e6g6eGe6gGg6eGeGeGg6gGe6e6eGeGeGeGe6eGe6eGeGgGeggCHVv,.                                   vHmUmyvv;;::;;vv;;:,;vryyYVllvYl;:yXcUkUUAryrrAyryrrl;;;VAv,yOgOOZDZD#OZOZOZOZOD9ZOZOqOZOZOZD@ZCCb2jjcckUhUXwrTAUrv.                                                                                                                                                   
eGeGe6e6gGe6e6g6eGg6eGeGg6e6eGeGe6e6eGe6eGe6gGe6eGe6e6eGeGe6eGe6eGe6eGe6eGg6e6e6e6gGe6eGeGeGe6BG0Uy;,.                                ;yLyl;.     :vv;:;;;;    .;;YYVYyVh, ,0bXUUwrLArMyyVryv;vYV;l;:rGOD#DqDZD#DZOZODOOOO9DOO9O9DODOZOZqj2cccjmcmckHNXwUUHHh;                                                                                                                                                  
GeGeGe6gGgGeGeGg6eGeGeGe6eGe6eGe6eGe6g6e6e6gGe6e6e6g6e6e6eGe6eGe6e6e6gGgGeGeGeGe6g6e6e66Ge666eeebHV;..                              .::     .;VtkkC6@T;;,;,.     YV;;vvvV;  0ZXkUUyryryrVryy:;VTV;vv:A6GZOqZZDZOZZZDZDZOZODODDZO#DZOZODO0#CCjtmcHHHmmckHHcHHXy                                                                                                                                                  
eGgGeGgGeGgGe6e6e6e6eGe6gGeGgGeGe6eGe6e6e6eGe6gGeGe6eGeGe6eGe6eGeGe6e6e6eGeGgGeGeGe6eGeGeGgGe6eOCXr;;.                         .;;,     ,yCqOGe6eGeGDjcNy;;;.    .v:;;;;;;;lctHkmm2y;;vvvvvv;;lyv:yL:vegO#DZDZOZDZO#O#DZODOZODD#OOeee9OZDZDZD#@bbjtHckmHHkUryVv;.                                                                                                                                               
6eG6GgGeG66eGeGe6g6eGe6eGe6g6eGe6eGg6eGgGe6e6e6eGe6eGeGeGe6gGe6e6eGe6eGe6e6e6eGe6e6gGeG6G6GeGe6Gp4yv,.                     .,,       vACZZt2ZOC2tpjqDG9gCY:;;    .;;;vvvv;vAHcHHmt2A,;vvvvvv;;vAY;y4; v8GD#OZD#OZOZD#OZOZOZDOO#OOZtbqDOODOZO#O#D@ZZ#tHUHkkN4AhLv.                                                                                                                                               
eG6Ge6eGgGeGg6eGe6e6e6eGe6eGe6e6eGe6eGg6eGgGe6e6eGeGg6e6e6eGe6g6eGeGe6e6eGeGe6gGe6eGeG6GeGgGe6e90hyv:.                   .;,.   .;rkCZOZ0jjC@b2mkX4tOOgeZl,;v.    .:;;;;;vAUpckwUcpAv;vvv;vvv;;lyvlLv  v8GOZOZD#OZOZOZOZDDOZOZOZOpUylYc9eOOZDZODDqZZZpmkHUkh4rv;,                                                                                                                                               
6e6e6eGe6e6e6e6eGe6eGeGe6gGe6gGg6e6e6eGeGgGgGe6eGgGeGgGeGe6e6eGe6e6e6e6e6e6eGeGeGe6g6eGgGeGgGe6OpkY;,.                ,;;,.  :r2bC20b@2pmtt0bCbCjttqZOO6Zy.:;VAy;,.,,,.,:vVXmjkTrtbUv,.;;;;;;..;vl;,;;  t89O#Z#DZOZD#DZOZO#OZO#DZGZty,,UO6OZqDZDZZ#D#ZpjmtHXrwl;;v                                                                                                                                              
eGeGe6e6gGe6eGeGe6eGeGe6g6eGg6e6g6eGeGe6e6e6g6eGeGgGe6e6g6e6eGeGeGeGgGe6g6eGeGgGe6g6eGeGgGeGgGeOCUy;:.                lv;;vyHC9O@ttkmHpjjcpCCCZO99OZOZO9GV.,;VcXAv;,:,,,,:vrmcmrAHjky:.,;;;;;  .;v.  vy ,4BBGDDZDZZZOZD#O#DZODO#OO9O9M, vtBeDZD@ZqZ@D#O#Dq#CUywylvv;                                                                                                                                            
6e6eGe6eGe6eGe6e6eGg6g6eGe6e6eGe6e6eGgGe6e6e6eGe6e6eGeGg6e6eGe6e6e6e6gGg6eGe6e6eGg6eG66eG6G66e69jkyv,.             ;vyYANmtjjtkmHtctc0##C@Z9DODOOGOD@D#G2; :;yAV;,,,,:,,.vrkXHkUXHttAv,,;;;;;,    ;,  ,V;,m8B869ZOZO#OZDZOZODOZZZDZOO6tv .MgeO#Z@Z#Z#D@DDDZD0cAAryyA;                                                                                                                                           
g6e6g6g6e6eGeGe6gGe6g6eGeGg6eGe6e6gGe6eGeGeGeGe6eGe6e6eGeGe6gGe6e6e6eGe6e6e6eGg6g6eGeGeGgGgGe6eZjAv;.             ;A4M4HjmcHmkmHcmtcCC#Cq09DOZODOOOZZ#Oqr. ,vMr;,,,.,.. .vU4kkmHHU2CHl;..,:,;,.   :v;   vv;v4jO9GDO#OZDZOZOZOZDqOZODOOBOr  vcOOZZ#ZqZqZqZZD#Dqbcjhyyw;       .         v                                                                                                                        
GeGeGe6eGe6e6e6eGe6e6e6e6e6e6g6e6e6e6e6e6e6e6eGeGeGgGe6e6g6e6g6eGe6eGe6e6eGe6e6eGe6eGeGeGeGg6eGOcyv:             ,mt4UHtcHHmkmHccjcp2q0@CDDOZOOOD9D##96@v. ;YUr; .....  .vUckmktmHtCcMv; ..,.:.     :v;   ;. :yZOGG9ZDZO#DZDZDZD#O@ZqDOB6H; ;bDDqD#Z@Z@ZqD#Z###Z0jyvvv.       .        j,  .                                                                                                                    
eGe6eGg6e6eGe6e6e6e6eGeGeGe6eGe6e6e6eGeGeGeGe6e6eGgGeGeGeGe6e6e6eGe6eGgGeGe6gGe6gGeGeGeGeG66ggeZtLv:.            vUk4NcctHmHckcb#q@COOOOOZ9ZOZOZO#DZ9OgH. ,ycwv    ..   ;rmUhAXNhTUHcHmX. ,.::.      .LV     .;VhcGg6OODOZO#DZZZOZZZO#OOeDX;lCO#Z#Zq#qZ##C@##0ZqOZtv :V;                                                                                                                                        
6eGe6eGg6e6e6eGeGgGeGe6eGeGe6eGe6e6eGeGg6g6e6gGeGe6eGe6e6eGeGeGeGgGeGgGe6e6e6eGeGeGe666gGeGeGeGOcL;:.           ;TUUhUUmHmmcHHHZZZZDZODOOODOOOZD#OZO9qy. :yrV;          lHHhvrAryrAU4Hcj:   .:         ;Vv     ..;AO6B66OOZOZZZOZZZOOOZODZcHcO#Z#Z#Z@Z#Z#Z@Z@Z#ZqZjXv;Vr,        .,.                                                                                                                            
e6gGe6e6eGe6eGeGe6e6eGgGeGe6gGgGeGeGeGe6eGe6eGe6e6e6eGeGe6gGe6eGe6e6e6eGe6eGgGe6eGeGeGeG6GgGegeZcl;,,          .yXUhUNUXkkmHmHH0OZD#ODOZ9Z9DOZO@O@G6Cr:  :v;.           wckL;lTrArTTXhUkhl,     .        ;V,      ,;yHbOe6gOOD9DOO9OOZOOO#DOGDO@Z@ZqZqZ@Z#D#D#ZqDCCtcAHkV.        Uc     h@                                                                                                                     
6g6e6eGg6e6e6e6eGgGeGe6eGg6e6eGeGe6eGeGgGeGeGe6e6eGeGe6eGe6eGgGg6eGeGeGe6eGeGg6gGe6eG666GeGeGe69cy;:..         Yh4hkUkXXHjcCj@@9OOZO#DZZZD#OZO#OZOGOA.  ;v:           ,A4UhTyLAwrMMUXXXmtDjv               vA4;      ,;TjZ@GGeG6GGOOZOZ9DOO9ZOZZqZ@Z###D#D#DZDq#qZ@#2cHjXV.       .qU;;m88BD;                                                                                                                   
eGg6eGe6e6eGeGeGe6e6e6g6g6eGg6g6g6e6eGeGe6eGeGe6eGeGeGe6eGe6g6e6gGe6g6eGgGe6e6g6e6e6e6gGeGe9eeeqcY;:.          rkTUHtHkUbOOZDZOZOZOZD#ZZZZOOgeO@OGCV.  .l:            YkNrrrMrwTNTNUUXUkctbjY                vhVl:       :vTkH20eBe96GgGeG6O9OOZZ#DqZ@ZqZZZ@DZZq#qZZDCcmqkv         tB8B@y:;w;                                                                                                                  
6eGe6eGe6eGeGeGeGe6eGg6eGg6g6g6e6e6eGeGe6eGeGe6g6eGeGeGeGe6eGeGg6e6e6e6eGe6e6e6e6gGe6eGeGe6gGeDjhY;:          ;wNXkcHHUUtO@DqZ@OZOZODDZOZOZOqOjhkg0;   ;.            ;MXwMLMrMMUXkkmkk4HkHH0tv                 ,vVyl;.     .  .vLkmCOGOZDGGegBee9O#O#D@OZO#Z##@Zq##O99@CZtv          te82;.;Yrrhwy;.                                                                                                            
eGeGe6g6e6eGeGe6gGgGeGeGeGe6eGg6e6gGe6eGeGe6eGe6e6e6gGeGeGe6eGgGeGgGe6gGg6eGg6eGeGe6e6gG6Gg6egDcNvv,.        ;MA4UHHcktcqOOZO#ODOZOZOZZZDZOmAvvvNH,   ,             :UthwAwAMNUUUTAXccmmcHcHj4h;                  :vkcA;.           vvlvYYrMkpO9eeGOODOZDZO#DqZqZqDZ9DO90l.         .yZZOMVvrrhUp0cV;                                                                                                           
6eGg6e6e6e6e6eGeGeGe6eGg6eGe6eGe6eGeGe6e6e6eGeGe6g6eGeGeGeGeGg6eGg6eGeGe6eGe6g6e6e6g6e6gG6Gegebmrl;,         ;vywHctc2ZGO9DOZOZODOZOOO#D#O#AyTwm.    ,              ;HmXyAyyVLYv;;.,;rwptjttHHHmN:                  .vYyYlv;::              ..  ;je8eB99OOZOZOq#qZqDZDZe#y.       :DGmmCbCtXAAAkmH44lv;;.                                                                                                       
e6e6eGg6eGgGe6eGeGeGe6eGeGeGe6e6g6eGe6e6eGgGg6eGe6eGeGgGeGeGeGe6gGe6eGe6e6eGeGeGeGe6eGe6eGgGBGCUrv;.         ;vvrhmcjbOO9DOOOZODDZDZO#Z#OO9jOBc    .               yhUXlvvvvv,.:vyY;;,:;;vAUcjpcHhA                    ,v;vvyyylv;;,.          .  ;rC6BB8BBGOZOZZZO9G9e8b:       ;ZB8Z0tpCChyyr4hvvvlvvv;,.                                                                                                     
6eGeGe6eGeGe6eGe6g6eGe6e6gGe6e6eGgGe6eGe6eGeGe6e6e6eGg6e6eGeGe6eGeGe6eGe6e6e6e6eGe6eGeGgGgGeeGtXlv;          ,;;Lmcjc2#ODOZODO#DZD#D#O@DDeeOpv    ;;              Vbjjhv;;;:.,yj09O@CcAv;;::,YwmjjjjLv                  .  ..:vvvvvvvlvyvvv;:;Ywy;,   :yc9B8BBBBBBB8B8B8;    .   Tegjjjpj0kVvVAjXrYVVYYylv;,                                                                                                    
e6e6eGe6g6g6eGe6e6gGeGe6g6eGgGe6eGe6e6e6e6e6eGe6eGeGg6eGg6gGgGe6gGe6e6e6eGe6e6eGe6g6eG6GeGGGeDtMlv;            ,hqqtct#O9ZOZODZ#OZODDZOGBBBv    :;.              AZ0jr;.,.. vHZZ#2C2CCbcmUXY. :vATt0Zcy:                       ,;;vvv;vvvvyvvvyyrlYv;...,,;vM4tttkcHbhv,  ..     VG#2cjjpjjhMyTXMTkUUAyVyVyvv.                                                                                                  
Ge6eGe6eGeGe6gGe6eGe6e6e6e6e6eGeGeGeGeGg6eGeGg6eGe6eGeGeGeGe6e6e6eGg6e6e6eGe6e6eGg6e6eGgGg6eeOtXVv,            :kOOZtjC9DODOZ9ZODOZeGOqeOV    .;:               vODv    ;vvVt90Cjjcjtjj2tjpbA;;;;vlAhUHbv..                      .,;;;;;;v;vvv;v;vvvVyVVv;,,             .;:.,.. mBZpCpbjpb0bmAyvrrUU4VyYyVyv;                                                                                                  
e6eGeGe6eGe6e6eGe6e6e6e6e6eGeGeGg6e6eGe6eGeGeGeGeGeGeGe6e6e6e6e6eGgGeGeGeGeGeGe6eGeGe6gGgGe6B@twy;.            Yg2OD#C@#DZOD9OD#DZOOeBec    ,v;;,              yUy; ,:,;t0q0@pptpcjtpjCbbp0Cqckllv:.;vyUCZZXl:                         .:;;:;;v;v;;;vvVlVYylyyv,     .;vYvA;.., .jB@CbCjCbqC2mNyMylvYVvvlvVlv;,                                                                                                 
6e6eGg6eGeGg6e6e6g6e6eGeGeGe6eGe6e6gGe6gGeGe6e6gGeGeGeGeGe6e6e6eGgGeGgGeGeGeGe6eGe6eGe6eG6GeBZHAY;             j6b0DZOZD#OZD#D#D#OOBGc;    ,vyv              .Ur;  .;VvXq##O0pj0pCCq@D#@pppCbqbcXMv;;;,,vLAkUccy.       ., ....          .   :;v;vvvVVllvYVAhXNhArVLVAwmcjCV.,.. pe#jCpCpCpCtmXhNNYvvvvvvvvvvvv:                                                                                                
eGg6eGeGeGe6eGeGeGeGeGe6e6e6e6gGeGe6e6e6eGe6e6eGe6g6eGeGeGeGg6eGeGeGg6g6e6eGe6eGg6eGgGgGgGegB0krl:.    ,;     lZZbZZODZqDZZ@OZO9BBBm     :lvvl;             rXv.:   vX0@btjcppCpCpCCCjbjbjpcjtppCtcU4Al;;;vYMMHtOCv       ,.. ..:::,:,.       . .,;;;;;vyLMThrVvvvvlAkchyry;,:: ;9ejtjC2C2Ctjmtcjj0mVvAlv;vvvvl;                                                                                                
6eGeGg6e6e6e6gGe6eGe6eGeGeGe6g6e6eGeGgGeGe6eGeGeGeGe6g6e6e6e6gGeGeGe6eGe6gGe6eGeGe6eGeGe9gGBGjTyv,.    :v    vC@jOqD#Oq#qD#ODGB80U     ,;v;;,             ;G@;    ,VU09Zttctjqb0bqjcUkHccppjtjmtc2CqjjXALALlvvlTUbD#4v       . ,vv;v;;;;:;;;,,,,.,...,.vlylYvVVVVrMUUkrv,  ,,;,.k8DmtbbqbCpjcjtjcjCbAyUUv;;vvvvv                                                                                                
e6eGg6eGeGg6e6eGgGe6eGeGeGeGe6eGeGg6eGe6e6g6g6e6e6eGe6eGg6g6e6e6e6eGeGeGeGg6eGe6eGeGegeG69ee6mTV;,      ;   .cZCOO#bC#ZqOOZ98BOw    .;v;vvl;             y2y     ;HZ0jbjjctcbb0bCCpUhhUUkHcHkHjctcCCCcHUmkmhrv;:, vN0COjl.      .:;;vvvvv;vvvvlvvvvvlvvvv;;;;vyyryVlyywyl::,::,.cBZmpb0bqb0tjcjmtcjptVMUV,;vvvYl,                                                                                               
GeGeGeGeGeGe6gGeGg6gGe6e6eGe6eGe6e6e6e6e6e6e6e6e6e6g6eGeGe6gGe6gGeGeGe6eGeGe6eGe6e6e6eGeG66BZUyY:         ;llHjpqGZbHcOGO6ODA     .vv;;vvY.             LDv    ,yb@Zp2ttmtcjccmtmcHXNUNUhUNXhkkcHtt#OG#Ctpj02jAVv,   .vCG6CUv,      .:;vvVlllyVyVArhMwTAlvvvvVVyyLlYYYvvlVv;,:;vc9pjjCC0Cq2cmjctcjtbtArhv,:vvlvyv                                                                                               
eGe6e6e6eGeGeGe6eGe6e6eGeGeGe6e6g6e6eGg6e6eGe6eGeGeGg6eGe6gGe6eGe6e6eGeGeGgGe6eGe6eGeg6G6Gee0Myv,      .,,k4UcjcjDG@cc9Cqcy      .;;;;vv;             ;c2     Vk#ZCcptjccmtcHrhUHkmU4wXhUNUNkNUUkUUUtjbpptjjbjtXXkck,  .YUttGZHyv;;   ..,::;VyylVyTTMATyyyryLyryLyryAylvvvVv;.;vHCjcpj00q0bkcmtctctcttb4v;;vvvvVv                                                                                               
GeGeGeGgGe6e6e6eGeGeGg6gGeGe6e6e6g6eGe6e6g6eGe6eGeGeGe6e6e6eGg6e6e6g6eGe6eGeGe6e6e6eGe6eGee9tXlv:      ,,lmUcjtpC9#jHjGCy.   .vAVv:;;v;              Ue9;    v09DqjjtpjjctmjUAyAAhTMAwNXNUkH4UUk4khUwHHccjttc##0t0@CUXv   .VUcH2@DtkAVv;,..;vvvvlVyyyryyVyVyyryryyVryrvvvvvv,.:vN#jpjCC@C#CbHcccmtctmj#bAv;v;v;Yv                                                                                               
eGe6eGeGe6eGeGeGe6gGe6eGe6eGe6eGg6gGe6eGe6e6e6eGeGe6eGeGeGe6e6e6eGeGeGe6e6gGe6e6e6gGgGeGe6e@cMlv:       .;mUcjZZZDZjtkX;:,;;vvYvYvv;;              ,p6T   ,:,te@bpCpC2btcHcmcNMAwrTwU4HkHkH4UXUUkUHUHmtmcmcHcj#@qccHtj0btrv:,;vYT4Hc2jjcUTNrvvvvvvVYVVyYVlylYvvvvvvvYl;:;;;,;:vvhC#D9#qq#0qb0mcHcmtmtc#CUvv;;;vvv                                                                                               
6e6e6e6eGeGeGeGg6e6e6e6e6e6e6e6eGeGe6g6eGeGeGe6eGeGeGe6e6eGe6gGeGeGe6eGg6e6e6eGeGe6gGegeGegqHhvv.      :YrUHH#OOZD@0tmv;vyVyvvvvvvv;.     ...    :ceCY   ;yccbCptbb0CCCpUUUmUUhNhXXUUkkkUkh4hNNUUkUHkcmHkmHmHpZZq@bbbq0@2jUMvlv;;vvLw4Utj2tcUhrrVVyrVyVLyryrlv;vvvvlYv;vvvvv;vlyAjq9GO0#0q00ttctmcmcccjZtAv;;v;vv                                                                                               
e6eGeGe6g6e6eGeGe6gGeGe6g6e6e6e6eGe6e6e6eGg6eGeGe6e6eGeGe6e6gGeGe6e6eGe6eGe6eGe6e6gGeGeGe6Gj4rv;,      TjcccbZDq@#OCcUryLVrryvvvvvL,  ..,..    ,Tcj4; .;;vXc#qb2Cj2tjt0jkUHkHhNhXhXwU4kUHU4MXh4UHkmHk4UUHmcmmt#b##GO9OOOOq2kbptXAVlvvvYVTAhhmcpcmXU4UNU4k4kUUMN4UX4Xkkjjct0pj2#2bpq#9O#2CC#2CmjjjmHHcmtqOtAv;;;vl.                                                                                              
GeGgGe6e6e6e6e6e6eGe6e6eGeGgGeGeGe6eGe6e6e6gGgGeGe6e6g6e6g6e6e6e6e6eGgGe6gGe6e6eGe6e6g6eeeOtTLv;.      htcHcbq202Z@HrAyylVvyl;;;;;:   .;;.    ;b6Cv;;vvvyU4HcpjCpjmcHccckHkmHmU4wkUHUHHmUHUHkkUHHmHHUUhk4HUkUUhhTkpZq#0Z#@CCpC2CtcUkhNMAVVlyrHcjctctctctcjcjcjjbjjmtp##Oj4A;.;TtjcmCDO0#@0p2jccCjtHmHmmp0Cmkv,,;V;                                                                                              
e6g6eGeGe6e6eGeGeGeGe6e6eGeGe6e6eGe6eGeGe6e6eGeGeGeGeGe6gGgGe6g6eGe6e6e6e6eGe6e6eGeGe6eGe6OcXYv:.    ,:AHUmt@Cq0ODqNMlYvlvllv,;:;:. ..,:,   ;VwHUyvvlVYvwHp#Dbjp2ccHcHcHcHcHcmtHmHmmcHmmcmcctmmmmHckkhUXUUU4UNhTNNNMwA4pbpqCCtjcjjjtjttm4MwrM4mHcHcmcctcttjcjjjtjccHX:        vVVlUG8eG9O@ZqbcctjccHmmmHmmjtU;.,V;                                                                                              
6eGeGe6e6eGeGeGeGgGeGe6g6e6eGeGgGeGeGe6eGgGe6g6g6g6eGeGe6eGeGgGeGeGe6e6eGgGe6eGg6eGe6eGgG6@HrY;. ; .XZYLkcUjj0CpctkryLlllVly;:,:,;,;;;,  .:lkqbclvYyrMyMmpCZ#CtpjjmcmcmcmtmcttmmmcmHkcHmUUkU4HHmkHkk4UNUhkUkUkUkUcctttcCCCbCjtmcmbb0C@bCjjttHmkmHcHcHcmtmcmcctmtctHcHHHHUpkHC@UylAkZO6GeGeGeOZpcHmHmHcHcHcHtjHv;v;                                                                                              
eGg6e6gGeGe6eGeGgGe6eGeGe6eGe6g6e6e6eGg6gGgGeGg6e6eGe6eGe6gGeGgGeGe6g6e6e6eGe6e6eGg6eGe6eG0kwv;,;Hyk8e..HOjkcctHUyYvVVyvvvvvv;;vvvVv;,. ,vMUkHtHkAU4HkmtZq@q0ctmjcmkHHHctmttCjk4t#9ZOOGcX4kNXwUNU4UUkUkUHkHUHkHkHkHkmkmcbjbjCCqC#0#0qC0CCCCpjmmkmkmctmtmtctcjcttpttcjjqZqtttjkMytO@NwCe9G96G699@jUHmcmcmcHcm2bkvlv.                                                                                             
6e6e6e6eGe6eGeGg6eGeGe6eGeGe6eGeGeGe6e6e6g6eGeGeGgGe6e6e6e6e6e6e6eGg6eGeGg6eGe6eGe6e6ggeG9HXMv;ymmCGB9: UOmkkcmckwlyyyyv;v;vvvvYvlv;::,vrUmpHUTTwmHcmcHpqZCqptHcctkkUkXctjctctkHmpC#@@CZO999OZ2q00tcmmHckHUkkHkHkHUkUHHmHmHjj#ZO##C@q#CqC0b0b2cjcjc2jjtjtjttcjtjc2tpj2jpcpjtmmUcCDbHhqOODO966eeBDqjjmcttmcHmcpt4vv                                                                                              
eGg6eGe6e6gGe6gGg6eGe6eGeGg6eGeGgGgGe6eGg6eGeGe6eGg6eGe6e6e6eGgGg6e6g6eGeGeGgGg6eGe6eGeggZHylvVvVcOb@OHlmtcHcHcHUMyYyyL;;;;;;vvvv;,,;;lmbUkkXwwMwUHUHUHcq0bbptttjmHUHUkHtcjmHHCCcUkUHUkj#0qC#cUNUAkmcHHkHUUXkUHkmUHUk4HHcmcHcHmHtjCCZ000qbCbq2Ctptjtjctcjctctcptpjptbjjtjtjtjt0ZOqbUkkcHc09DO9ggBeOCpkccjccHcmtjXv                                                                                              
GeGeGeGeGgGe6eGeGe6e6e6eGe6e6e6gGe6gGe6eGe6e6eGeGe6e6eGeGeGe6gGe6eGgGe6eGeGg6e6g6eGege6e6ZHVvVyvv#D0j##ZCpjbcjkUUkALvvvvvlvYvvvvv;.;yXUjctHHkmkHkkUmj0bOD@tckmkmHHUUkHUccjcjHcp#cHkHkHkmkcmmmtUrYwj0jbjjttUHmccpjtXkUUktccHmHmUHUmcjjC20bCC@Cq0qC0pptjccmjctctctcjtjtjcjtjtD9GDZbtHHHHkmUcCbcZOgGgGe9bHtcjctHcmpZy                                                                                              
eGeGeGe6eGgGg6g6g6e6eGe6gGeGe6eGeGgGeGg6eGe6eGeGe6g6eGeGeGeGe6e6e6eGeGe6e6eGe6e66G6G66Be9jhYAMHkrHDC02@ZOr;ybp4HmkMvv:;VArryyVyllVTNmctmcHmHtcjcjmcCO#@Z9#pmk4UNk4kUkUkUcctmjctcjcjmcHcmcmttC@jjZODbCbbpZCqbbcjj#q#qZZCHcmtcccjttmtmjcjtjtpCq##0##Z#ZqZq#C@#Z@qCCbqCqpmUk4t0Z0@jkUHkmmcHckcmcCOOODgB8D2mcmcmcmccZl                                                                                              
6e6eGeGeGe6eGeGg6e6eGe6eGe6eGeGe6e6gGeGeGeGe6e6e6g6eGe6eGeGeGe6gGe6eGeGe6e6eGg6eGgGeGeee0kyyHpOerybZ000#tV.;4XMpmU;:vvlryyllvvvvvj0pUU4HkmHHkcmcHmkC##qO9Dbjkk4kUHUkUk4kHtcccptptptjctmcmccccZ9DckwXwUhkpqC0pjjpcjt2jjcmkcHmkmHmUUw4hNXUXU4HmmHjj##D#OZOZOOOO9OO#@C#0Cctct2CjbCCkHmcHcHmHcHtcbpCb0qeB8G#CbmHHcHt0y                                                                                              
gGe6eGeGg6eGeGe6e6e6eGeGeGeGe6eGeGeGeGe6e6eGeGeGeGe6gGe6gGe6eGeGeGg6eGe6gGeGe6eGe6gGeeBZjwlyD889Vyq0qC0ppc0qc;:AH4vvhAMryVyVLVyVMCOtmUkHmHmkccjcjmtp@bqCqpCjcHckHkHUkUk4HmtHcccctcjc2jpttcjmHkpHUhU4HUkUccpcbCZ0cwhMwAhUHkHHk4k4kNhAhwhwhMhANMhTHcbjjttcjcjp@qDZZ0#C#@OOODO#CC#CmkmmmmcHcmtjptbjCjptb9B6g9Zpmkmmbcv                                                                                             
6e6gGeGeGeGgGeGe6e6e6e6e6e6eGe6e6eGe6e6eGeGeGeGeGeGeGeGe6eGe6e6g6eGe6e6eGe6eGg6e6G969eO04y;.l86CrVcZC0C0pbpZCMlNhUhU4UwMyyyXtpccmcccHmmcmtctmpb0bqbq2CbtUUXkHcHcHmHmkHUkkcHcHcmcmjcj2@#ZpjctHmkmkctjtptcHHHtjCjCjckXXkXkkkUmHH4kXUNNhXhkUkUUhXMwMhNUh4hNwXhkkHHcj#@Z0#bqCqbjctcjmttjcckckmHHkpp000pCj2G8BeOG2mmtp0kv                                                                                            
e6e6eGe6eGeGeGeGeGe6eGe6eGeGe6e6eGe6e6e6eGe6eGe6eGeGe6e6e6e6gGe6e6e6e6eGeGe6e6ege9G9G6#kTv; ;OGZHvcZqbqbCtcmbbCckXTATATyArLAmmmkmHmmcHccjctmmtCC0C@CCp0tUAAMUmcHcHckHkHUmHmkmkmmckmHHkHkccjtcHmkmHcctttkckmkUMwNkUHUkUHkcmmkHUUhUhkX4hUUmHmU4hXTN4kUkUkUkUkkHmmHmmccckmkHkHUHkHHttjjbtHHcHHUccq@#q#Cqb@OOZ9D9D9ODCpw,                                                                                           
6eGe6eGe6eGe6e6e6g6eGe6e6eGe6e6e6g6gGeGeGeGeGe6eGeGeGgGeGe6eGe6gGeGeGeGg6e6eGgGe6e969eCmrv; ,b9Bb;yDZCjbjjmccjjbtmTwhNTMLTrwkmHmmcHcmccjcjttt2C@0#bCC0CCUhrwwmHmHckmHHkmHmmcHcccmcHmUUwUmptjHkUtpcmtctmmHmHkU4hUwUkkkHHcmtmckkUkUk4HkHUkUmkkXUh4hUUkkHUkUHUmkmHmUkNUXkHtmtctccHmc2jCppHHmmkccjCZZ#0@CqC#@O@NrcjckkcpY                                                                                           
gGeGe6eGe6eGgGe6eGe6e6e6eGeGe6eGeGeGe6eGeGeGe6gGg6eGe6eGgGe6g6eGe6e6g6eGeGeGeGe96G6GgGOmUlv ,ceBD;vDG2bcptptjtjtjkkNHUkcjjptjccmtccmtctcbpbpqC@qDZCmjtpckhUUHkmkmHmHmHmkcmttttcccccHckmkcbCHkM4COcmctccmtccHcmtctmtmcmtctcjcjmmkHHmkk4kUkUkUU44h4Uk4mUkktHHUk4HUkw4hXwkHcmtHmHmHtpbpCjcMXwyvNkjqDC#CqC@@D@cV;vylvvAjCV                                        .   .                                             
GeGe6e6e6e6eGeGe6eGe6eGgGe6e6e6g6e6gGeGeGeGe6e6eGg6gGeGe6eGeGe6g6e6eGeGeGeGeGgGg96GGGeOjwyv..Ue8Z;vOGqp2tjtjtjccmcHkUkUtcjctctcjctmjmccjjCjbb@@DZO0HkHHcHmkHHmHmHmmmHmkmmccp2CjpmcHcmtHcm@qm4Ar#OjkcHcmcctcjctcjmcHmHcHcmtctccHHkmkm4U4kUkUkUkXUNkUmkHwHjjUXNkUkUUNUhXwUXUUkUHUHctmjttkwAMvvvTNjq@0@0Z0DZOp4yrN4AVvvUmv,.. . .                           .     . .   . . . .               .                    
eGeGgGe6e6e6eGeGeGe6g6e6eGeGe6e6gGe6gGe6eGgGeGe6eGe6egeGe6e6eGeGe6eGe6e6e6gG69g9GGG9eg#kwl;  y8BOArmjC@bCpCtpctctmmUmUkHcHmmcttctmcmcmtcjjqZO2ccttcUcHmHckmkHkcHcHcmcHmHmmjcpjbj2ttmcHcmc2@mUAAb9tmmcccmtcjttHk4cmcHmctmtctctmcHmkmkk4k4k4HkmU4UHUHkHUhUttchwhUhUXUXkhUXUXk4k4kUHXNhU4XAwMAvlywm@##0@qD#9OOtkUtmUL: vXXv, . . . . . .               .           .   .                                           
Ge6e6gGe6e6e6eGe6g6e6gGe6e6eGeGeGe6gGg6eGg6eGe6eGeGeGgGe6g66Gg6eGeGeGeGgGeGg9696O9O9#ZtNrY:  ,@8OZcrw@q#0qCCpjtjcccmc2cjmcmtctmcHjC@20ptc0@9qHNk4kwUUkUHkcmcmmHHUHkcmcHmHccjtjtbCqbjtcHcmtcmkkXccckcHcHmHcmcmULrhbtkkjmmmcmcccHmkHHHmmkmkHUtt4MctckHkHXHHtcUwHkUUkXUh4XUXUUHUkXUhwATrTMwMhAAVMThtZCqb##OZOOOjkXHNhvvvVAl,. . . . . . . . . .                   ... . . . . . . . . . .   .                      
g6e6e6eGgGe6e6e6gGeGe6e6e6gGeGgGe6eGeGeGeGeGeGgGeG6GeGgGeGeGeGe6eGe6e6e6eGgGg9G9GO9ODbcrVv;  .m6GCUAm0@CqC@0qCjtpctmjj2cjtjttHHkmpqbq2@qZ#ObmUHkH44XhwhhkHmkckXAwhkHtmmkHkcmcmjpbjjtjtcmcHmkcHmHckHHHkmHmHmkmkttcHtjtHmkmHcHcHmHHkmHcHmkmkmjjrwc2HHUHUHHcmcHcccHcHH4kUkhkUkUHNXMhAAAMATAwTA;vrAVMkccC0Z0bCOObXAMX4Hhv;;;:.... . . . . . . ..... ... . . . . ... ... ..... . ... . . . . . . .                   
Gg6eGg6eGeGe6eGeGeGe6eGe6eGeGeGe6eGeGe6e6e6eG6GgGeGg6gGgG6Gg6eGe6eGe6eGe6eGgG69GOG96OZmAlv:. .U6O0mDGOZOq@b0Cqpjcptjc2tjtpctHHUHmC2020#9OOpUXkUkX4XU4NhXNkUHUUTTAhwmmmHmkmkcmckHkHkmmtmcmcmcmtmcmcHmkHHmUmHcktCCckUtckkmkmkcHmUmHmUHHcHk4HmbtXwjtmkmkHHcHmmtmtcjctmHkHkmkHUHUUTwrArArAAMrwrvvlvvvNmjpCCCjCZ9jkrMMU4wv;:::, ..... . . .   . . . . . . .     . ... . . . . ... ..... . . . . . . . . .            
eGe6eGg6e6e6e6e6e6e6e6g6e6eGeGgGgGgGe6e6e6e6gG6GeGgG66e6eG6Ge6e6eGe6g6e6e66Gg9G9G99DZjUvv;,   V060qDGO9DO#0202bcccpcptpcjcpjCC0C@C@C@qZCHhhANhUNU4UXUhXwUXHUkUNAwrNHcHcHHkmHtcc4UUHUmHcHcHmmmHcmcmcHckmkmHmHmHmkmN4NkXkccHcHckTU0jwTcmmXhUpjtkmcjctmcHmHHUkkmHtmccckckmkHkmUHUHNNTXMwMAyryryryyVLAcj0q#b00OD2krrwhUMyvv:;,... . ... ... . ... . . . . . .   . . ... ... ... . . . . . . . . .   .               
6eGg6eGe6gGeGeGeGeGeGe6e6e6gGg6g6eGe6e6gGgGe6eGeGgGeGgGeG6Ge6e6gGeGeGe6eGeGeGgO9O9OZcmyv;;.    U6q0OOOOOOO##0Zbtmtctmjtptjmb#O#OZO@#@9#jTMhUUU4UXUXUNXhUXUkHHHwhMwNHHmHmUmHcHcHcmcHmHmkmmmkmHcmcmtmckcHmkmHmkHkHHccjmcmtpptjcchmqChwHtmHUHmtcttjttctmcmjttkmHtmcctccHmHmkmkmUHkmkHHcmm4AYyVVlVllvVhjjC2b2CbjctcjmUNhMAv;,,,. . ... . ... ... . . . ... . . . . . . . . . . . . . . . . . . . . . .              
eGeGe6eGe6eGe6e6eGeGeGgGeGe6eGeGe6e6eGeGeG6Ge6eGeGgG6G6GeGe6e6e6eGeGg6e6g6gGeG9O99G#2mrv;,, ..:H8##ZO#DZOOGOG9DjtjCcpcjcpcttC0#ZODZcpjjkXXUXUXUXUXUUUh4NXhUUHUkX4hkUmkHUHUmHmHHmcmmkHHmHcHmkmmtmtctHckmHHHmkmHmkcj0bbpCj2tjtpjccOZHXcHmmbppUtctttctctcjbOZZZDCpcjcptcmcHmUHmmHmHcmmHcmcXNhUwwrwAvvwcb2Cp2pjmjcpjtXwrTAlv;,,,,.. . . ..... . .   . . . . . . . . . . . . . . . . . . . . . . . . .               
6eGeGeGe6eGe6g6e6eGeGe6e6e6eGg6e6e6e6eG6GeGgGgGeGgG6GeGgGg6e6eGg6eGe6g6e6e9eG699OeGZjHyv;;,....LGegBe6OOOG9GGe9OCCp2jjt0#DZDDOO9O9p4MhTUXkXU4kUU4k4UNXw4w4NUkkUHkHkkUHUkUHHmkmHcmmHcHmkmHcHcmcmjcptckmmmHHmbjHm2@@tjtptpcjcttjtpjbmHHmUHcCjmHtctttctcjctcppq0qbbt2j2ccctH4UcmcHmmmUHUmkcHtcmUUkmyYUtc2j2tjccmtmtHNrryAhHwv.,,. ..... . . . . . . . ... . .   . . . . . . . . . . . . .   . . ..... . . .     .  
e6e6eGe6eGe6e6eGgGe6e6e6g6e6e6e6eGeGe6e6e6e6e6eGeGege6eGeGeGe6e6g6e6e6eGe6gge6G9Gge0jUyv;::.    v4p9696GGOGOG9gGZC#CbjC@9OGDOZZ0#0jAwAhNU4k4UXkUUXUUUhUNUXUXHUkkmkmUHUkUHUHHmHmkcmcHtccHUktb2HcmctjmmHmmHUjqq4cCGOccjcjctcjcjtbj2jjtjttcjtjjjcjtpt2j2tjccHcmHHjttttctmcmtmcHcmcmckmmcttmcmckUNkHAymcmt2jjtp2CtjcXyrhcjOZtv:,: . . ............. . . ... . . ... ... . . . ... . ... . ... ............. ..... . 
6eGe6g6gGg6e6eGgGeGeGeGe6e6e6e6gGeGeGeGeGeGe6e6eGe6e6eGeG6GeGeGeGeGeGe6gGeGeGeG69e6OjHVv;;,.    .,,MC9Oeeg9696GgO99G0CpZD9OGqcmcHckUMwNUUUXUUU4UXUUkXU4UUU4UUkkHUU4HUHUkUHHmHcHcmcHmmtcHXHj#jckcHtHHUHUHUHc@tkH0OOmccjtjctcjtjcpjjt2j2jbtpjbjpt2t2j2p2jjtcmmkcHtctmtHcmtcjttctctcctbpbtcHcHUNUUkrNHcHjcttpj@@@0#khwp@##Dtl:;,  ........... . . . . . . ... . . . . . ... . . . ............. . . ... . . . . .  
e6eGeGeGgGe6eGe6e6e6eGe6eGeGgGeGe6e6e6e6e6gGe6eGeGeGe6eGeGg9eGe6e6eGe6e6e6gGeG69geg0chVv;,,.    .. .ytqOBGG9G9eGOZOCjpZDqjCCCHXMhTNwXTUXk4U4UUk4kXUNUXUXUUUXHUHXXAwANhUUkXHHcmcmmmtmtctkUkttpcmmtctHHkc4UUq@jHtp#tcHcccctttmcHcHmkmHcHcctcjmjcjtjtjtpj2jbjpctcpjpjbbbctcjtjtjtjcjcjt2tm4kUXMhUNrAwccccpcjtjb@Cqqqcc#0c0bw:,;; . ......... . ..... . . ... . ... . . . . . . . . ... . . . ... . . . . ... . . . 
6e6g6e6eGeGg6eGe6eGg6eGg6eGe6eGe6eGe6eGeGeGe6eGg6e6e6e6e6eGeGg6gGeGeGgGg6e6eGg96GBDmrVvv;.......,;;:.;vUGe9GOGG0kHAVrHjZjHmjjjhNwUXhTXXk4UXk4kUkXUXkNUXUXU4UUkUXAMAMAhhkXUUmmcmcmcHHkmUHHmkmctcpj2bCjbp0tCC2cjcbtcHmUHcjj2jcmmkkUkUmHmHmUHUkHjtjtcHHHHkmmpjqbCC@0Z@ZC0Cb2bjpttmtctctcckwAXArlYvvVXmtHjcjtjtjC02qCpcjmmctV. :;;,. . . ....... . ... . . . . . . . . . . . . . . . ... ..... . ... ....... . ...  
e6eGeGe6eGe6gGeGeGeGeGeGeGeGg6eGeGe6e6gGeGe6gGe6eGe6e6eGe6e6g6e6e6e6e6eGeGeGeGG96e9chyv;;.,.,.,.,;;;;.,lee9O9OOjkrLyTwkUHTN4kUXhUXUhXTUwUXUUkXUUkXUXUXUXHXUhkUmmtmcctmmkmHcmcmmmcHUNhTN4mHmktt2pbtbjC22bq2jkcjpjjmccttC2020b2ctccHckHmtjjjjtCC#q@bpctmmU4rAhHkjp2jb2C2C20bCttHmmcctctmtcttjkhyVlVNmUmctmtmccpjbccmtmcmjmY,  .;:.... ... . . ..... ... . . . . . ... . ......... ... ......... . . . ........... 
6e6e6e6e6e6eGeGe6e6e6eGeGeGe6gGe6e6e6e6eGeGeGg6eGe6e6eGeGe6e6e6eGe6e6e6eGe6eG66gOObUyYv;:,.,.,.,,;;;;;,vZBOOOO2HAyrwwXwwwXAhMhhUX4hUhhTXXHNUXkXkUUXU4UUHUkUHHmcCbZZDZZtcmmHcHcHmmcmjtjmcmcHmc2tptcHmkHUHm00D0D#@pjkHktcUNkUXwwATrrVvvlYyyyYVyMAMrhryyLyyvv;YrATXT4hkcjcjtC22ctcttjmjct2ZZZCbjthAyXXUctctmcmtjtjtHmkHUkHkYv;.  .. ........... ... . . . . . . . ... ... . . . . . . ... ..... ......... . .....  
eGe6eGe6e6e6gGe6e6e6eGe6eGe6eGeGeGeGeGe6e6eGeGgGeGeGeGeGg6e6eGe6e6e6e6g6e6e6eGeGObcAyv;;:.,.,.,.;;;;;;;;CeD0qtkrXmUTXwXw4w4whTXXXhkhNTNNUUHUUXU4UXUNUXkUkUHkcmttjjbjbtcHcUmkmkcHcHkUHkHHcctcccmHck4hkUHHctb2C2CtHAryVv;,....,,:,;::...:......,:,:,,.,.,,::,;;;ryyVVVyLTMNTXXccjmcUkUkktjC22ttttkhrwhUmtmtccmtctkUw4NNThwUXAv;   . . . ........... ....... ... . ..... . ... . . ... ... . . . . . . . . ... .   
6e6e6eGe6e6eGeGe6e6eGeGe6e6eGgGe6eGe6e6eGeGe6e6eGe6e6e6eGe6e6gGeGeGeGeGe6g6eGgeeZ@cXyl;;,..,.,.:;;;;;;;,4e2tHNLUtCU444hXh4X4hUXUXUX4hXhXNHkk4HUUXUXUXkUHkHkcmtctHkXU4kHmHmkcHcmcHcUHUHHjtptpUXAwANUmHcHmUmUHkMlv::::,.           ...   .     ..,.,,::;:;;;;;;vAMrAyyvvvvvVVMNU4khNwUN4X4T44k4HUkAryrYTttmcmtccmUXkXUwhMU4HXAy;.  . ....... . ....... ... ... . . . . . . ... . . ... . . . ... . . . . . . . .  
e6e6eGe6eGe6eGe6e6e6eGg6eGeGeGeGgGeGg6g6e6eGeGeGe6eGe6eGgGe6g6g6e6eGe6e6g6eGeGeGDtmAVv;:,,,,,,,,;;;;;;;,lZjUwYrXjcHkHkH4kX4NUN4hUwXTwhhMNUHUmkHUNNXwkUHUckmHtmcHHUHUkUmcjctmcHcmtctcpcchTATATAhMwAUUmk4hyvv;;..               .    ...... ..,::,;:;:;:;;;;;;;vvVTAMMMylvvv;;YYyVAAwUHkHTYvllLrAyrAwrrMtmtmttjtjHmkHUUXXXUUcHTl; . ..... ... . ... . ..... . . . . . . ... . ... . ....... . . . . . . . . . ..  
6eGeGe6e6e6e6eGe6e6eGe6e6eGgGeGg6eGeGg6e6eGe6eGeGe6gGe6e6g6e6g6eGeGeGe6e6e6e6ee60mhrv;:.,,,::;:;;;;;;;;;;ccHMyrkktcmHHkHUUw4h4NkXUhXw4hUUHkmHcmcmcHcmcctmtmjmcmmkmHHHmmttjtjmH4UXkUAyUrvvlvlvyAwATrylYvv,                      .  .,,,...,.,:;;;;;;;:;;;;;;;;v;vvMXcj0cwlv. .;vvvvvvvvvvvvv::v;vvyVArwtbpbCqqZbjttctccUH4mcjMv;,.......... . . . ......... ... ..... . . . . . . . ... . . . . ... . . ... . .  
eGeGg6eGgGeGeGe6e6e6e6e6eGe6eGeGeGeGeGe6e6e6e6e6e6e6eGeGgGe6e6e6e6eGgGe6e6eGe6BGCkAlv;,.,.:,:;;:;;;;;;;;;ycUTy44U4cmckcHHUUhNNUhHUmmtcckHHcmcctctcjmcmtmcmcHcHcHmkHkHHHNXUUX4rrLrvv,,;;,;:;;;,;;;;v;;::.                          ,.,,:;;;;;;;;;;;;:;:;;;;;;;;v;vlMwktjUUAY,..,;vvv;v;vvvvv:,.,.:vv;v;vvVvYvvvVyrrAAwTUUkUcmHyv;. ..... ..... . ......... . ... ... . . . ... . ..... . . . . . . . ... . . .   
Ge6e6eGe6gGgGe6g6e6e6eGg6e6gGe6e6eGe6e6eGgGe6eGeGe6eGe6e6e6gGe6eGeGeGgGeGe66Gg99jUryvv,,,,,:,;:;:;;;;;;;:ykkALNmXXUjHmHmUHUUhUkmHccjtjttHmmtttctHcHckmkHUHkmkmmtcjttHcHNLrVVvv;vv;,,.,.. .         . ...             .     ..:,:,;,:,,;v;v;;;;;v;;;;;;;;;;;;;v;v;vvlyHccmjpmMrvv;vvvvvvvvvvLyVvv;;;;;;;;;;:;,;;;;v;vvVyALhhUXUrl.  ,........ . ..... . ... ... . ... . ... ... . . . . . . . . . . . . . .....  
eGe6eGeGeGe6e6eGeGg6eGe6g6eGe6eGg6eGe6e6g6e6eGe6e6e6eGe6e6e6eGeGeGg6e6eGeGg6eGObmyvvv;,,:,:,;:;;;;;;v;;;;YmUNM4kkTUmHkHkkUHX4UctpttmcmcctmtcpjjHmkkkHkHkHkmHmHccbjpU4XUwyvv;;,::;:,.,...        .                .,.,.,:;:;;;;v;;;;;;:;;;;;;v;vvvvvvvvv;;;;;;;v;vvv;vYAwmm2O#4UMvvvvv;vvvvvvVVAAV;;,;vyVyVyAUUUrryyVryrYyyNThTU4l.  ......... . . . . . . . . . . . . . . ..... . . ..... ......... . .......   
GgGeGe6e6eGeGe6g6eGeGe6eGeGe6g6e6e6e6eGeGe6e6eGgGe6eGeGe6eGeGe6e6eGg6gGeGgGe6eO0kMlv;;,,,,,;;;:;;;;;;v;;,vkkUmNkkU4HUkUUNUXUUHmjctmcHcHmccmcmcHmUkkmkmkHHcHcctkXkmXVvlvvvv;:,....,   ..,       .,,          .    ;;;;;;vvvvvvlvvvvvv;vvvvvvv;;;;;v;;;;;;;v;vvv;vvvvvvlyhNmcCjj4rvvvyYvvvvvv;;;;;;;;;;vvvvVLXkcHHkkXXLryryrAhTX4Hy;   ......,.... ... ... ..... . . ... . . . . . . . . . . ....... ......... .  
e6e6e6e6g6eGe6e6eGeGeGe6eGeGeGgGeGgGg6g6eGe6e6e6g6e6gGgGeGe6gGeGeGgGeGeGgGe6BGDjkrVv;::,:,::;:;:;;;;;;;;;;UXkUUUkUkUU4kXUXUNmkcmmmcmcmcHcHmHmkmkHkHkmkcHckckUMyvYlVvvvvvvv;;:          .        ,.          ;,..,;v;v;vVMyMMAYyVyYYVyVAyrrUv;;;;;:....,;;;;;vvv;vvYvvvyAkkcXc0HyrLrryvv;v;vVlvY;:;;:;,::;;lLUUmcckmkXLyyryMMNwX4wl:   . ..... . . ..... . . ... . . . . ... . . . . . . ... ....... . ..... ... 
6eGeGeGeGe6e6gGe6eGe6gGg6g6e6eGeGegeGe6eGeGgGeGeGeGeGe6gGe6e6e6g6eGg6g666eGgGZHhYv;;::,:,:,;;;;;;;;v;v;;;;THkmkmkkUHkcHcHkkkUHkkkHkcHcmmHmkmkmkHkmkHkmHcHHhMVvvvvvvvvYlVlVVV:,                           ..:;vvllYYyVrwHHmUmjjjqjHXcct0ChmbC2CHUV;.......::;;;;;;;;vvvvlVUUhhpCkvvrc4vvvv;;lvllYykAv;v;:   ,;vVHHmmtUhyryyyAAwwUUw;...   .....................................................................  
eGg6eGe6e6e6eGe6eGeGg6eGeGe6e6e66GeGgGgGgGe6e6eGeGgGe6e6e6eGe6e6eGgGe6g6eGG6gbkAlv;::,:::,;:;;;;;;;;;;v;;;AUmctmcmcccccmmhhw44k4kUkkcmtmcHmUk4HkmHHHmkmkHhAYYvvvlvYvYvvvvvvvvvv,  . ..                    :;;vrAwXHXHUmNHcm4kkHkHhUmjp6V ;cOeGZjcUNAMrr;:,::;;;;;;;;;;;;V4kTwUCqM;v4tryvl;;;vvvvvlAyyYyv:.,.::vyHkkXUNUMTAryMMhXkTv.,,,.. ................................................. ................... 
GeGe6eGe6e6e6e6gGeGe6g6e6eGeGeGeGe6e6eGeGe9e6g6e6e6g6gGe6eGeGe6eGeGgGe6eGe9BeZcNlv;;::::,;:;;;;;;;;;;v;;;;yUcjttcckkX4XUXhMh4cj2jptjmtmcmcmtctttmmkHUHXk4rlYvvvvvvllvllyrAyrYVvv;vv;;v:..,.               ,:;vvyyyvvvv;;;v;vvYYrywUkjOOrvHjmCpXArkmtmjtUl;.:;;;;;;;;;;;vykkXTmj@jy;vTH4Uryvvvvvvv;;vAmmklv;;;;,;yHk4XUhUUcXhyYlyLNl..:,:......................................................................  
g6e6gGe6eGe6e6eGg6eGe6gGeGeGe66gBe888BBge666eGe6eGe6e6eGgGe6e6e6eGeGeGe6eG6DqmXyl;;::,:,:,;:;;;;;;v;;;;;;;yNtt2tckkXUUHkjpC0#b2tbjbccmmmcmbj0qCjHMAyArrLyvllVvvvvvyyryT4HUkkUNU4UTNyrLv;;:, .           .,;;vvvvv,..,.,,:,,:vvv;;;rH@bC#OZt4UrVvvlhTArwTrvv;;;;;;;;;;;;;vwHMwHjbZmv;vwmHHAXlv;vvvvvvyAhUUMAll;, ;yUXmtpkUXkNl;v;;wh,..:.,...................................................................... 
6gGeGeGe6eGgGeGg6e6e6e6eGeGeGeGBB8BgO9Oeee6e6eGeGeGeGeGeGeGeGgGgGeGeGe6e6eg@mUVYv;::,::;:;:;;;;;;;;;;v;v;;vAUjjpmmktjbCq2Z#D0jUhThhmHmkcmjjbCbUTyyvvvlvlvYvlvYYrLAXkX4Twkc4XhUUctq0Z@DUrrVv;;;.        ..;;vvvvvv;.,,::;;;:;;v;: .;kmUh4X4MAlVvvvYrMyrAhLyvv;v;;;;;;;;;;vyAyVAXtjtLv;lUkNwkMvvvvvvvvvvVyyrVYvlvvvvvyNbCcNU4XAry;  v;.,.,...................,.,..................................................
eGeGe6eGe6eGeGeGgGe6e6eGe6eGe6BBtvv;;VHO8ge6e6eGe6eGgGeGe6e6e6eGe6gGgGgGeeBDjkTVv;;,:,;:;:;:;;;;;;v;;;v;;:YyUHjjcHjqDZZ0mN4hXTMAArXmtHmHtcqCZmrvv;;;v;;;v;vvvMmjbppccmcHNLUHUMyvVVyyArUk4AArv:,..      ..,;;vvvvlv,,::;;;;vvyvv,:;vv;;;;;;:,,:vvYywwNTXNNyvvvvvvv;;;;;;;vvVlyyNXmHUv;:VrAXtHHMyVyyVvvvYVMyVvvvllVvvvvvAXUkUhNwkw.  .,...,...................,.......,.,.................................. . ... 
6eGe6e6e6g6e6eGeGe6eGeGeGeGe6eee;  .;;ye88ee66Ge6eGe6eGe6g6eGe6e6eGeGg6ege9#UTyv;;,;::,;:::;;;;;;;;v;v;v;;;lAHctmptjtjHchMhwNHkHUHcqbCjjHtcptUYv;;;;;;,:;vVT4ppbjpccHmqO; y@Z0Yvvv;;;;;vvMhHv.          .,,;;v;vv;;;:;;;;vv;,,.       .,...;;vvvvyLArryvvv;;;;:;::::,::..;vYywhkkmhL;;yXrktjkUAwh4MLvvvlywyVlYvvvlvlvyXHkkXXNUUmU;  ;. ..,.....................,.....,.....,.,.,...............,...,.,.,........
e6e6eGgGeGgGeGgGeGe6e6eGeGgGeGO9v  ..  ,mB8ge6eGg6eGeGe6e6e6e6eGe6eGeG6GeZpXAYlv;;:,;,:::,;;;;;;;;;;;;vv;;.vkUmmctpmckHHccccjpqCq0qCqCqjmHcmcXAv;;;;;;:;Vhj0qpHhrrcccHjkvv4bpMv.:;vvvvv;vvYAv.          .,:;v;v;v;;,. ...               .,:;v;;:;;v;;;;...,.....,......... ,vlMXmkUyv;ywNHtmkkHkHkkArVlvVyrVyVVlVvYvYrMhcHkUUUHNmt; :;. ..,.......,...,...,.,.,.,.,.....,.,,,,,,,,,.,.,.,.,.,.,.,.,.,...,.,,,,,.
Ge6eGg6eGe6e6e6eGe6eGeGeG6Ge6eZGy,      ;OGDGe6g6e6e6eGeGgGeGe6e6eGeGeGeeGjHhrlv;;,:::,;,::;;;;;;;;;;v;vv; vUcHcHmkmmcmjbq0q0Z@D#O0qpbppmcmtcULv,,;;;;;VU2qDqUYv;rtptjtHkmbpNv:  ,vvvYllv;::..         ..;;v;vvvvv                                 .     . ..:,,,,...,.,.. :;vykkkhAYv;lAmmckHHmHkrLrTrYvvvVlVYVvvvllrLhkmUkUHkXXjl  l;  ..,.,...,.,.....,...,...,.,.,.,.,.,.,.,.,.,,,.,.,.,...,...,.....,.,.,..
eGeGe6eGg6gGe6e6e6gGeGeGgGeGe6G9bM;     ;wO06geG6GeGe6eGeGe6e6e6e6gGe6eeBOjUwyvv;:;,;::,:,;:;;;;;;;;;;vvv;.vUkjttmcmtcpp0Cq0@CqpCtptptjmckhwmTy;. ..;;vYhHt4rv;,,;YvlvYvv;;:,....  .,         .       ..,;;;;;;;;:                                        ..,,... . ... ..:vYrmcHhAyyv;vrUmkHkmHmhAyAwUAVvvVLrAVlvvvVVMNmHH4kUHN4Ul  ;l,  ,,..,.,.,.,.,.......,.,...,.,.,.,.,.,.,.,.,.,.,.,.,...,.,...,.,.,.,.,.
6eGe6eGe6g6eGe6g6gG6GgGe666e6BgBBGm      yC098eggeGe6gGg6gGe6e6eGeGgGeeeCcTylv;;;;;;:;:;:;:;;;;;;;;;;v;v;; ;lyXtmjjCpCC0C#q#0tUmUkHHkmmmXhlMHhV;.  .:;;vvrMwvv;;.,:::;:;;,                   ...      ..,;;;:::;:,.,,.                                 .   . . .         .,;;VHjM4wyvvvvyUHmkHHmmchyyXt24YvyMHUAVY;;lyrUHckHUkkmXUmV  ;v. ,;.,.,,,.,.,,,.,...,.,,,,,.,.,.:,:,,,:,,,,,,,,.,.,.,.,.,.,.,.,,,,,,,.,
eGe6eGeGeGe6e6e6e6eGgGgGgGeGe6e6BBBw      vU6BB6gGeGe6e6e6e6gGe6gGe6g6eG#mUyYv;;;;;:;:;:;:;;;;;;;;;;v;v;;:,..,vy4k0@#0q0#qZ@@hLAALhrwAhArVvyHAY;,...,:;;;vyyylYv;;;,:::,,.                    .       ..;;;;;,;,:,,,:,,                                         .         .,.;H2MNcrvvv;vhcHmHcmtkUylAjjcrvvrwmMVvv;vVrwHkHUmkHmcUjpk;;vv:. ..,.,.,.,.,.,.,.,.,.,.,.,.,.:,:,:,:,,,,,,,,,,.,,,.,.,.,.,.,.,,,.:,,.
GeGeGg6gGeGe6eGe6e6g6eGgGgGgGeGggBejv,     ;jBBBGeGg6eGeGeGeGe6eGgG6Ge66qcXAYv;;;;;::;:;:;:;;;;;;;;v;v;v;;...,,;vMNcjHNUUhwhMrYyyyllvlvvvvvvvv;;,,.. ..,.,;vvYvvvv;vvv;;..                     .       ,,;;;;:::::,,,:,.                                 .       .   .    .;.;U@wkmM;lv;vhmckmUHkHUhyNmcHUyyAhX4Arv;;yLhUcHHHmkmHHHcCqv,v;...,.,.,.,.,.,...,.,.,...,,,,,.:,,,,,,,,,,,,,,,,,:.,,,.,.,.,,,,:,:,:,,
eGeGeGeGgGe6g6e6e6e9eG6Ge6eGege6eeBOtU:     vpeeeG6GeG69gge6eGe6e6g6gZqpHALvl;;;;;;:;:;:;:;:;;;;;;;;;;,...,...,,;lAUcNTrMrAyryyrMyvvvvvvvvvv;., . ........;;vvv;;;;:;::,   .            ..... .       ..:;;;;::::,:.....,...                                              ,,,;Up4Hpr;lV;vrcmckmkkUUXkkHUHHk4UNXNUhAYlvlywXmUHkHUHkmkmth. .,,:,,,,,,,:,,,:,:,:,:,:,:,:,:,,,,,:,,,,,:,:,,,,,:,,,,,,,,,,,:,,,,,,,:.
Gg6eGeGeGe6e6e6e6eGeGg6e6eGe6eGegeeBCCk      lmgGG9GGgGeGeGe6gGeGeGee9@bkwVYv;:;;;;;:;;;:;:;;;;;;;;;;;,..,.. .,;;lLhHUhhwNThATrAArllvlvvvvvV;.     ..,.  ..:,;:,.,                       ... .         ,:;;v;:,;,:,,.,.,...,.                                            .,:.;TbUctA;yLvvyUjccHmkHUUXkUkUmhXhUwhrwXUV;;vVrAUkHkcUmHcHtph,..,,:,,,,,:,:,:,:,:,:::::,:,:,,.,.,.:.:.:,,,,,:,:,:,,,,,:,,,:,,,,,:,:,,
eGeGg6g6e6gGg6eGeGe6eGeGeGeGeGeGeGeGOO0v     ;v2gO99696966e6e6e6e6geeDZckMyvv;;;;;;:;:;:;::;;;;;;;;;;;:...,...;;vvlvLXXhXwwMNwhMyvvvlvl;v;v;;                                           ..... .       .,::;;;,:,:,:..........                                            .,,.:Ajmmjr;YrvvycccmmkHkkhHkkUHUUwU4UwUNXNy,;vlVrwHHmkHHtcjtpcA;,,,.:,,,,,:,:,,,:,:,:,:,:,:,:,,,,,,,,,,,:,:,:,:,:,,::::,:,:,:,:,:,:,,,
6e6gGgGeGe6e6eGg6eGe6eGe6e6e6e6egeGeg8Bc      .N@G96G6966eGg6eGege66#jkUVyvv;;:;;;;;;;;;;;;;;;;;;;;;:;,......,;vvyrUUUNXTXMNwUkXv;;vvlv;:;;v;                                          ..... .         ,,;;v;,,:,:.. ........                                            .,::;lkXtCr,vlvvAUmUkUkHclvH0HkhAUmkmkkkkN4NVvv;;vXmcHmHcctcjcmr;.,,:,,,:,::;::,:,,,:::,;::,;,:,:,:,:,,,,,:,;:::;,;:;:::::;,;:;::,::;::
e6gGe6e6e6eGe6eGe6e6g6e6e6e6gGeGe6G9ee89y      ;4GgG6GGGeGe6eGgGeegDDtHAVlv;;;;;;;;:;:;;;;;;;;;;;;;:;:, ... .,;;lrX4HU4wXwNwXUmy;;vvvv;:;;;;;.                                          . .   .        .::;;;.,,:,,.. . . ... .                                       . . ,;;vyrht6L,vvvrMkUUwXTttv;c0tHwLHHcHmkkXUXcjmLv:;vMAX4HHcmcHtmV..,:,:,,,:::,;,;:;:;:;:;:;:;::::,:,:,:,:,:,;:;:;;;::,:,:,;:;,::;:;:;::,
GeGe6eGe6g6g6e6eGeGe6e6g6e6e6eGeGgGGGee8t;       w96DOC66e6eGe6egBeeObHhyyvv;;:;:;:;;;;;:;;;;;;;;;:;;;.. . .,;;vvrhkUUh4wUhXXkTv;vvYlV;;;;;v;.                                           ..           .:,;;v:,,:,:,. . ....... .         .                           ... .,;;vVhNbj;;vvVrU4UNUXMcpv;mbmHMyNmkmkkUkhwmeOjhyvly4UtccHHUHHtMv::::,:,:,;:;:::;,;,::;:;:;:;,:,:,:,,,,,::;:;:;:;:;,:::,:,:,;:;:;:;:;,:
eGgGe6e6g6eGe6e6eGeGg6eGgGe6e6eGgGgG69geec;      .AG69O6eGe6eGgGgegCthrlv;;;;;;;;;;;;:;;;;;;;;;;;;;:;,. .. ,;;vlMTUUUNUh4X44HA;:vvYlVVl;;;;;:                                             .           .,,,;;;,:,:,: . ..... . .         . .                          .,..,;;vvAXCZh.vlYyhXkNXXUAtU,:C#tUhrmmcmcmcHmUkmC0twr;;;vlVrUmmUmHCN;:;;;,:,::::;,;:;,;:;;;:;:;;;:;:;:;:;:;:;:;;;:;:;:;:;:;:;:;;;;;;;;;;;:
6gGg6eGeGe6e6eGe6e6gGg6eGg6gGgGgGeGeGgGeB82V       4GBeeGe6eGeGeGeDCkhyl;;;;;;;;;;;;;;;;;;;;;;;;;;;;:,   . :;;;yMXNXhXNUUkhkHr;;vllYvVv:;;;v                                             ...           ,,::;:,,,,:,....  .   .                                       :,,.,;;;VrN@9v;lVVATUUUXk4Hth ;qOjcwhHjccmckmHtXMkCHMyv.:;;;vTcHkHmtA;;;;;;;:,:,,,:::,::;;;;;;;;;;;;;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
e6eGeGgGe6g6e6eGeGeGeGe6e6e6e6e6e6eGeGgGee#r       ;kOeeeggG69GGgOOjcXrvv;v;;:;;;:;:;;;;;;;;;;;;;;;;:.     :v;vlTNUhXw4XU4UUkv:;vlyvllyv;,;v                                      . .   .....          .,,;;;,:,::: ...                     .                     . .,,.::;;vlrwqZv;lvVyhXUUHUHmqy ;@#bm4AHctmmmckmkUAmpcTAVvvTrrLAAXUmmHhr;;;;;;::.,,:,:,:,;;;:;;;;;;;;;:;;;;;;;:;:;;;;;;;:;:;:;:;;;;;;;:;;;;;:
Ge6eGe6eGeGeGe6eGg6g6eGe6e6e6eGeGgGg6gGeG6Zbv       vb9eeeGe6GOGOOjhLYvv;;;;;;;;;;;;;;:;;;;;;;;;;;;,.. . . ;;;vLwUU4wNhUXUXHhv:vVyyyVLAy:.:v                                       . .  .. .           . ,::::,::;,....                                          ....;:;;;;;;vVU#Cvvlyyw4kMmppNHU; VqCtcmcmcmcmtctmckUUcHXhNyVywAAVlrHtqHUHr;:,;;;;;;;:;:;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
e6e6e6e6e6e6eGeGe6e6eGeGe6eGe6gGe6gG6GeG6Ge62v       MZB8ee6eG9@#CjMrvlv;;;;;;;;;;;:;;;;;;;;;;;;;;:.. ... .;;;vMkhUwNwXwXhU4T;;vMLLyLrwl:.:;.                                           ... .             .,:,:,:,,.,.:..                     .                  .,.,;;;;;;;vvvwDtv;vvAXcHrH0Cmhv.;UtcttbHcccccmtcjtjUkkm4XUHlvvyYYlUcA;.vUv  :vv;;;;;;;;;;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:
Gg6eGeGe6eGeGeGeGe6eGe6eGe6eGeGg6eGeG69gGee8B6Y      :Ne8B6eGeZCp0chrVvv;;;v;;;;;;;;;;;;;;;;;;;;;;:. ......;;;vhU4XwMNwNhXwkTv;vyyYyVAMV,.:v.                                            . .               . ..,,:.;;;;;                       .                 .,:,;;;;;;vvvYj@Hvv;vVXmcAccmXV;vkjkHH@q4HCHtctmtcjHUUHkkhkHh;vVyVVkp.  .r.    .;:;;;;;;;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
e6gGeGe6eGe6gGeGeGeGgGe6eGeGe6eGe6e6e9eGgGegBBq:       YDeegeeOpbHAYvv;;;;;;;;;;;:;;;:;;;;;;;;;;;:, . . . ,vvvAUkwNMhwNMNM4Xhv;vyyyyAywrv,;v;                                           ....            . . ..,.,,::;,;:                                        ,,:,;;;:;;vvv;rCCrYvv;YAmcmUcyL;;X#ckkcq#4cCtmcmcctcmUkUHUTMUV;vVVLymtY:YkU;     V;,;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
6e6e6eGe6eGe6eGe6e6eGeGg6g6eGe6e6gGe6e6e6egeeBBp;       AGeBBB@qjMvVv;:;;;;;:;:;:;;;;;;;;;;;:;;;;,   .   ,,vvyTUNNMhTwTNwhhULvYhXXLALrAwv,:v:                                            ...               ..,.,.,,;;;;,   .                   .   .   .     . ,:;:;;v;;;vvvv;vkUyvv;vlwkcccHryv;NtbmHHbpmm2ttmjctctcmUHUkUHhL;:;lyLhMM088th;  yUO4;,;;;;;;;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
eGe6eGe6eGe6e6e6eGe6e6e6e6gGeGeGeGeGg6gGeGegeg89N,      ,M9B89jHhv;:;;;;;:;:;:;:;;;;;;;;;;;;;;;;;,.  .. ..;vvlwXXTwMwMNMwTXhVvAXmMLyLyAAv,:;; .                                          ..               . ,.,...:,;:;,:.;;.                    .. .   . . ..,,;;;;;;;;v;v;v;vUUlv;;vyAmcjtmrVvvrUUjtjcjctcjtjcjctttHHHHUcjp4yvv;;v4AykGB8U;  cBO2Y;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;::,
6g6e6eGe6e6eGeGe6eGeGg6e6eGe6e6e6eGe6g6eGeG66eeBCH;      vU9e@UMyv::;;:;;;:::;:;:;;;:;;;;;;;;;;;,,   . ..::vvvrUTNTwMhMNMNN4VvL4Uhyryrrkv,,;;:.. ...                                     ..                ,.... .,:,:,;;;::..                   ... . ......,,;;;;;;v;;;v;v;;vrVvvv,vYhktttUrYryAAwcbjjcjttcjcttjcjccHmUHp#bqbXl: :wAV4ZBZNvkmmZhlv;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:;;;;;;;;;;;,:
gGeGeGe6eGeGe6eGe6eGeGeGeGe6e6e6eGe6eGgGe6gge6eeg9w      ,vHDbUArv;;;;;::,:,:,:,:,:,::;;;;;;;;,.. . . . ,;;vvvAUNTTAhAwTwMhNryUmtUALrAUkV,,;v;;......                                   ...                 ,.......,,,,:;;,,                   ..,...,.,.,,:,:;;;;;;;;;;;;;;;;;v;v;vlAwmtcXwVrTUwwAXHbttmcmjtjtjcjctcmkHm2bbCbAyv;,vYrTmchyr2p:..,,;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;v;;;;;;;;,:,::;:;::,:.
Ge6e6eGe6e6e6gGgGe6e6e6gGe6g6e6eGeGeGe6e6e6e6eGGOe2y.      ;HtwLv;,,,:,:,:,:,:,:,:,::::;:;;;;;..   . . ..;;vvvrUTwMwMwAhMNMhAM4tjcrwAwXHV;,;vv;;,. . .                                                     ..,..     ..,,::;,,                 ........,.,.:,:::;;;;;;;;;v;;vvvvvv;vvrAUmjkUAVyhANTMAUHkUkUtt02bttmtmckmHp20jbwyrk4l;YAkmtXU4HU; .,;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;,,.,.,........
eGe6e6gGe6eGe6eGeGeGe6eGe6eGg6eGeGeGe6gGeGe6eG6D9gGt;      ,rkrv;...,.,,:,:,:,:,:,:,:,;:;:;;;:. ... .   ,:;vlvVMUwwANMhMhThMhTwktcNATAXUT;::vvYv;,.   .                               .   .               ..:;YyyrTl;.,.:,:;;:                  ......,,,.,,::;:;;;;;;;;;;v;vvvvv;v;YrNXccmXTlLhTwhAwUHkH4kcbCC22ctmttckmcbpbtHX4XkAvvM4ccc#byv;::;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;;;;;;:,:,:,,.,.,...
GeGe6eGe6eGe6eGe6eGeGe6eGeGeGeGe6eGeGeGeGe6e6e6egBBemV     ,;vvv;,.,,,,:,,,:,::;,;:;:;,:,;:;:, ..... .  .;;vVVvrUHNNMwMhMXThAUwhmcwAAwTHw; .;lvv;;;.   .                              .. .,;,.            .;vVAcb9g2Lv,,,:,;:,                 .. .:,;:;;;;;;;;;;;;;;;;;;;;;;vvvvv;;lTh4UUhhrrrwMhMTAUHHk4UjjCCCpjctcjmckcjbjbt2bCcUrv;YktUjXv;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;,,.,.,.....,.,......
e6e6e6e6eGe6eGe6e6e6eGe6e6eGeGe6gGeGe6e6eGeGe6eGe6Beeqv     ..:,,.,,,,:,:,:,::;,::;:;;:::,;:;.....    ..:;;vvYLAkmkhhAhwhwXMwHHXcmXAwAhUh;. ;lyYvvv:,..   .                           ..,:vvv;,       . .,vvrwUmCckLLv;.,,,,;;,               ... .:;;;;;;;;;;;;;;;;;;;;;:;;;;vvv;;;yk4whrwAwThMMAhAwwk4UwNkctCb0jcHcmmkUkptpjpt0bbm4v.;UmHmM;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;;:.,.,.......... . ..... 
Gg6g6eGgGe6gGeGe6g6e6g6eGeGeGeGgGeGe6e6e6gGgGeGgGegg9eT        ..,.,,:,:,:,:,:,:,;:;,;:::::;,,..      .:;v;v,vANhUNhhUUUrrAhAcHcccUhwwhkXv,..lyyYl;,:;,. ... .                        .,,;;vkqmL:..,:;;::;;vvv;vVv;vvl;.,:.:;v:,               ....;;;:;;;;;;;;;;;;;;;;;;;;;;;;vvv;;ykwhMMrMANAMAwMNwXhXwhANktC@bjmkMXXUwHttcjtj2CcmhL;;lUmjw;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;v;;;;;;;;;;,..,.,.,...,..........  
eGg6eGe6eGe6eGg6eGeGeGe6e6e6eGgGe6e6eGeGeGg6e6eGe6gGeBOr:      .,,,,,,,,:,:,:,:,:,:,:,:,:,,,: .       .,;;v;:;TMhThhHt2UrvLrTHcHcHmTThNXUY;..;Lyyv:.,:,.,.. . .                        .,.. ;Mq2Mvvvvvv;;,;;;.. ,..,:::.:,,,;,:,.               ..::;;;;;;;;;;;;;;;;;;;;;;;;;;vvvv;;r44hkHkh4wU4UXNMhAXwhAMwHc@0@tcNAM4hkkcHmc2p2ccmmXl;vlUch;:;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:;,,,,,,...................... 
GeGgGeGeGe6eGe6eGeGg6eGg6eGe6gGe6eGe6gGg6eGeGe6g6e6ee8#U;      ,.,.,.,,,,,,,,,,:,:,:,:,,,:,:..         .,;;:.vhwATwhk2jmyvyTAcccmtHwrMMNNXv;.:vyVl:..,,,,,.. .                         ..,.    vvvv;:;;;;;;;,.,,.,.....,,,,,.,.,.               .,.,,;;;;;;;;;;;;;;;;;;;:;;;;vvvvv;vrkHpCDmUUkHcmmwMAwMwAwAh4ctCb0jpckXUUHkUUc2CccmjtjMl;:VbA:.;;:::,:,;:;:;;;;;;;;;;;;;;;;v;;;;;;::,:,,.............. .........
gGeGeGeGe6e6e6g6e6gGe6eGeGgGeGeGe6eGg6gGeGeGeGeGG6eeeCmhV:     .,.,,,.,,:,,.:,:,:,:,,,,,:,,,,            .;. ;UhhhXwHctUMVAATUcHcmcMrrMrhXv;,:vvyLrv:...,.,..   . .                     .,,.          ,:;;;;;,:,,,,,,,,,:,,,,.,., .             ....:;;;;;;;;;;;;;;;;;;;;:;:;vYllvvvwUctCtmUk4mcjmULAMhMTAwT4kttbb#0ZpkUk4UXHcbtcmtttNwv;,rbw:,;;;;;;;;:;;;;;,;;;;;;;;;;v;;;;;;;;:;,,,,,,...,..,,.,...... ......
GgGg6e6eGgGg6e6e6eGeGeGeGeGeGe6eGgGeGeGe6eGgG66gGeGO2UyAYv.    .,,,:,,,,,:,:,:,:::,:,;,,,,,,.,.         .:;   l#HHkHkkUHXUwNAwccctc4AArAhcA;,;vVrwryYv::,,,:,:,,..                       ..,.          .,;;;::,:,:.,,:,:,:,,.,........       .,,...;,;;;;v;v;;;v;v;v;;;;;;:;;vlVvvvvY4Ht2qpcUkUmmm4TAwwhThw4HtjCbCC02t4UUU4kUHkcctcpq@Hv,:VpT::vvrhXXUhhMAyLyyYVYyllvv;;;;;;;;::,,.,.,.,,,,,,,.,,:,,,,,,.,.,.,.:
g6e6eGe6e6gGe6eGe6g6e6eGe6eGeGeGe6g6eGe6e6gGeGgGe69tmUyvv;;.     .,,,,:,:,::::;:;:;,;::,:,,..,.           .   l@tHHUmUkUmH4ArAjcjcjk4AAywtmv::vlrMAlyLY;;,:;;;;:. .                       ...             . . .,:,, ..,.,.. . ..,...,.. . . ..:,,.,,:;;;;vv;vvv;vvv;;;;;;;;:;;vvlvlvyMHtZO#cHUX4mUUh4MMAww4NHkctq0qC0ck4HUkUHUmHjtjtD9qwv:v4w:;yTkCccctmjccHtH4wkXwAylv;;;;;;::::,:,:::,:,:,:,,,:,,,:,:::,:,,,:,
6eGe6e6eGe6eGeGeGeGe6gGeGeGe6e6e6eGe6eGe6e6eGg6e99pHwTvv;;::..   ....,,:,::::;:;:;:;:;,:,:....                vjHkUHkmkHHmXMyXjjtjcckXryycUVv;;vVh4Xrrvv;,,;;;;;....                         .                              .....,.,.,.......,,:,,,::;;;;v;v;v;vvv;v;;;;;;;v;;;vvVlVVTHbD9btUHw4UHkmXwANAX4UUkT4mqC#0#pjtpmckHkcctcjCZjmVvvXw;,cZOqcwryVyLvYv:.   .:;;;vvv;;;;:;:;:;;;:;;;:;;;;;;;;;:;:;,::;,,::
e6eGe6e6eGeGe6e6g6e6gGeGeGg6g6e6e6eGe6e6e6e6g6e69DjNrlv;;,;;;:.  .,.,,;,:,;:;:;;;:;:;;;,:,,.,,.               VkHkkU4XHUkUXrAhbttmtmcXAyNUXyLl;;VAUNUrvv;,,,:,:,,.,.. ...                   .                           . ........,.....,.,...,,,,::;:;:;;v;vvvvvvv;v;;;;;v;v;;;VYVlrAHt0Z@mHkUNUXUUUThMhTUUHkXhm0@0D#0jjtjkHMwHjmcHHcZtAvvNU;y9gh,.                      ,,,...:.                ,:;;v;;;;;;;;;
Ge6eGe6e6eGeGe6e6eGeGeGe6e6g6eGeGeGe6e6g6gGeGeG9qtUMvv;;;;;;;;.   ...;:::;;;:;;;;;;;;;;;:;:;;;:.            ;vTmkHUUXUh4X4MArhcjmjctmHTkmkAVvvvvvyMwMhTAv;,,.:,,,,.,......                 .                             ............,.,.,.,.,,:,;:;:;:;;v;v;vvvvvvv;;;;;;;;;;;;vyVLrwXctDqckHXUXUhUUXMwAMM44UhUjZb0C0jtcptmUhActcmHUcOOHL;rv;Vw;              .::.  .  .,,:,:...                       .;;;;vvv
g6e6gGeGeGeGeGeGeGg6e6g6eGe6eGg6g6e6g6gGg6egeg60bUAlYv;:;:;:;;;.    ,;v;;:;;;:;;;;;:;;;:;;;;;;;:,          vTyhUHUkUUXUhUhXrMAHmjt2jjcjtckAvlYryvvyrAAwy;,:,:,,,:,,,:,,.,               .   .                               ... ......,.,,,.,,;:;;;;;:;:;;vvvvv;vvv;v;;;;;;;;;;;vyrrryTXmCZjcUUhhT4XUAArArMMNwwTmpCj@0C2tHppmAhkjccHkmZOCM;.:;v,          ;vYUt2DZZC#qZpC2jccmkyV;                             .
6e6eGgGe6e6eGe6eGeGgGg6e6gGgGe6e6eGe6eGeGeGegBZpcUVvvY;:;;;;;;;;.    ,:;:;:;:;;;;;;;;;;;;;;;;;:;.      ;vvcCmUXUXmkUwUwhwUwwrTrhmCbpjpjCtkVvvyLAlvvryrrrvv;::::::;,:,,,.             . . . .                                   ........,,:,,,,,;;;;;;;;;;vvvvv;v;vvv;v;;;;;;;;;;;;vUrlvVVAHbp2khA4NUArLAyrMXw4whwtC0Z9GDkXjGkMAHctccktC@jU;.:vv;      ,vcZZ6B8G9GgOO22pCbCcbp2jmTAY:                            
eGeGe6e6eGeGe6eGeGe6e6gGeGe6e6eGeGeGgggGgGegg9#HUyv;vv;;;;;;;;;;v.     .:;;;;;;;;;;;;;;;;;;;;;;;:      yVY#0CkkUUctkUXXMwwXTTryycpC20bCb@mLvvvAATvvyyyrLMV;:;::,::;::..             ,.... .                                     ......,.,,:,,,:;;;;;;;vvv;vvvvvvvvv;v;v;;;;;;;v;;:v4AYVvYvywtqpMhhUwrlyyyVN4UN4wNHCCDOGqHNpOkrwNmHctpjpjbcr:,;v;   :r0e88B8ObUyVwAAyYYylYvv;v;vlyyMrl.                          
Ge6eGgGeGe6g6eGg6eGe6e6e6g6e6eGe6eGgGeGgGegeZDcAvvvv;;;;;;;;:;;;;.     .,;;;;;;;;;;;;;;;;;;;;;;v;,,:,.,rvvpCTw4U4ctCtpHHH4MTAMyXmCbCCCj0D@hyvVAwMAyrLryryVv;::,:,;:;:,..           ,.,.. .                       ..,   .         . . ...,;:;;::;;v;vvvvvvvvvvvvvvvvv;v;v;;;;;v;v;;vNMwrryv;vYUkXXk4hVVlVYVyXwNArMmmjqZqqjt2CNArhhmkjtjtpjbcr;vVrLHO69DZ0Ml;.         .  .. ..   .lrmcAv;                        
e6eGeGeGeGeGgGeGeGe6e6e6eGe6g6g6gGe6ggegeGe#tcklvvVvv;;;;;;;;;;;;:.     ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:;vv;UHrAkXkUctjjtmchhAwrAAUcq2Cjpb9Ob4AyryArMrAyAAXMrv;,,.:;;;;,,           ..,.... .                 ,.,.,,;;:,:,:.,,:,. . . ....,:;:;:;;;;v;v;v;v;vvvvvvvvvvv;v;;;;;;;;;;:vyUUckUY;,:vrrALLlVvllVlYvYlYvLhccCC0C02bcUrAMkHctjcjc2CpLvvvvwGBG#wALv,                 ..    ;hmmkwvv                       
6eGe6e6gGe6e6gGeGe6eGe6eGeGe6eGe6g6e6e6Be9D0Hmrvvvvv;;;;;;;;;;;;;vv;   .,;;;;v;v;;;;;;;;;;;;;;;;;vvvvvvv;vrkVwUkXwMmctmjtHX4hhLrAHp0bbpbqZpcwAyArArAAhXccjUyvv,.;v;v;;:;,.        ..,. ...       .,:,,.. ..,.,.,,:;;:;;;;;;;;;,.   ... ..:,;:;:;;v;v;v;;;vvvvvvvvvvvvv;v;;;;;;;v;;;LwmHcmhvVvvvvvvvVyylllVlVlYYVyThcpq0@CCjjcmAwUmmpjjttt0qmvv:.vjjcALvyArVrLLv;,,               vklvjj@2V                      
e6e6e6eGeGeGe6eGeGg6gGe6e6e6e6e6e6egeGeOO00HhLvvv;;;;;;;;;;;;;;;;;vv;      ,;;v;;;v;;;;;;;v;v;;;vvlvvvvvv;vLUkkU4wrrmUcC2kUTXhwVrwtqCpCjqCjHHTMAwATAATmHccjTrVv.;;;;vvvvvv:..     ..,..       . .,;,,,,;;;;;;;;:;;;;;:;;;;;;;;;::,,.....,:;;;;;;;;v;v;v;v;vvvvvvvvv;vvv;;;;;;;;;;;vYhkcmmXwcmAl;vvvymUUUUhrYrLrVLyw4cjbtjcctjmMAkkttpj2ttjOZNv;.vkqpUyvvv;;;vV4XUh:             ,r4v,vmpZ0y                     
6e6e6e6eGe6e6e6eGe6eGe6gGeGe6gGeGeGgGeOZpjkhvvvvvv;;;v;;;;;;;;;;;;;v;,     .;;;v;;;;;;;;;;;;;;;v;vvvvvvvvv;;AHkkXUrAwHjCmHwrrmUwrMU2j2pb2qccHXrwAMrMAwkcHckXArv;;;:;vvvvvv;;.........,.    .  .:;;::,::;;;;;;vvvvvvv;;;;;;;v;;;;;;;;;v;v:,;;:;;;;;;;vv;v;vvvvv;;;v;v;v;;;;;;;;;;;;;vlhHtHmNXmtUrYlvlyAXmmkAArLlyyAAMXcmcHmkcjjMwUHmjtptjmt@OUv,.,Yp9DpUHmcwTrNAwwkNv.          vAUkl,;VXG8U.                    
e6eGeGe6eGeGgGg6g6gGeGeGeGeGe6eGeGG96GG0bUAvvvv;vvv;v;;;;;;;;;;;;;;;;;;     .,;;;;v;;;;;;;;;;;;;vvv;vvv;vvv;rkmNkXUrAUjjjkAYycjUrVwtCjppCCbjthMrylyrUXkkkUUNXrv;:  .:;v;v;;,,...... ,,,.    ,,;;v;v;;;v;v;;;;;vvvvvvllVll;;;;;;vVlvvlYYv;,:::;;;;;;;;;v;v;vvvvv;;;v;v;;;;;;;;;;;;;vvvvAMXktmct2tchrYvvvykUHUH4TwHmtHccptjtp2##jhX4HkmkmHcHjcmXy;.;XCBeBB8Be9GGDcmmpUA;;,      vpjcjr,,vVqOk;                    
GeGe6eGe6e6e6eGe6e6eGe6eGe6e6e6e6eG696OjkTvvvv;;;vvv;;;;;v;;;;;;;;;;;v;.    .:;v;v;;;;;;;;;;;;;;;vvv;;;v;v;vvrTNUmkrrXUjtmLVyjtHryLpCCjjpCb#tUwyvlYATUkkUHUckUyY;, .,;;;;;;;.....,...:..   .:;;vvvvlvvllvlvvvvlwAwyryMANrVv;;;vhwMVlvvvv;::;;;:;;;;;;v;;;vvvvv;;;v;v;;:;;;;;;;;;;vvvvlvVyhHjtjtjtCckwTyryrAkUkXt0ZZDCbjbbq00CDjUNUUHUkUHkkUHHjjtv:;ycZpkhmjtjmUtpCjC2c20tjX4AXHtmcU; .;vyHCZ                    
e6e6eGeGeGe6e6e6g6e6eGeGe6eGeGe6eGeGOCtAAvvvvvv;v;v;v;v;;;;;;;;;;;;;;;;.     ,;;v;;;;;;;v;;;v;;;;;v;v;v;v;v;;;vAkkmr4kmt0cTYym2cUrTUHcpjbbq#tLlvvvyLMwkkHkkUUXXyV;;,;:;;;;;:,.,.,...:,,.....:;;;vvvvvlV;;vVvvvrtjcckmHtttXNAwAhNNALVyyLryvvvvv;;;;v;v;v;;;vvvvv;;;vvv;;;;;;;;;;;vvv;vvVYrTHcjcjjZqpccctUAlAhUwUkjp0CCbCjbpCp#OZkUXHUHHmmcHmUHc#OM;;;AjpyvYTlv;YhHkUUmcZG8g#2#bkVyyv   :;vreB;                   
GeGe6eGeGeGeGeGeGgGe6e6eGe6e6eGegBDqtHhrYlvvvvvvvv;vvv;v;v;vvv;v;v;;;v;:       ,;;;v;;;;;;;;;v;v;v;v;v;;;v;;;;;TkmkHHcmC#CAyympCHUAMMmjbC@0OHv:;vyYATXUHkkUkXkMNArv;,;;;;;:;::::.,.,,;:;vv;;;vv;vNpjmmNwwjCqbD9O#9OGO9O66DHcHtNVvVYLVLVTrAyVvv;;;vvv;v;v;vvlvv;;;v;v;;;;;;;;;;;vvvvvvVVyYhcpcjtqZhvyhjj2ctHHkkAUHctbbC2Cp2jb#6O0UUAMAmcpccUUkjCql, ;Aq@mrrAkUkUk4hMNwkkctjcbbHv       .;vwe8,                   
gGeGeGg6gGgGe6eGeGe6gGg6e6e6e6egeBDmkMrVyvvvvvvvv;v;v;v;v;v;v;v;v;v;v;vv;.     .;vv;;;;;;;;;;;;;;;;;;;v;;;v;;;;AHHmHcmctbjUVyk20#mAVATmtZOOOtv;;vyyyhNHkHUkUk4UwNry;:,;;;:;:;;;::,,.,:lyylvvYrhAtOBee9OCOB8B8B8OZ#9#tmjttkyvllv;;;rLryMh4ryllvv;vv;;;;v;v;vvvvv;;;v;;:;;;;;;;;;;vvvvvlVlylMUkThXM;:vUUkk4HtmcHUUcmccjjbp0tttqOeO0m4rAUC0cMNHZZbAv,:;A2BeZtA;;     vvAkAlywpCCcMv.     .:;Y9G                    
GgGe6e6eGe6e6gGeGe6eGe66GgGg6ee88BjmArVVvvvvvvvvvv;v;vvv;v;;;;;v;v;v;v;v;;.    .:;;v;;;;;;;;;;;;;v;v;v;vvv;;;;;yhkkHHmkmHchYvUtCDOUyVAhmt0pCh:.;LUyYyU4UwkkHUmUUArl;.::;:;:;;;;;,,.,:vvYlAXmcqH2ZO@OGe99DZp2p2HXrTTrv;:;;;;;;;;;:;vlvvvvyMVYvv;;;; .;v;;;vvvvvvv;v;;:;;;;;;;;vvvvvvvvvvllyrNVvvv:;vryvvVrhmjHtctctmccp2qCpHtbOGg#CpmAkbZcwwcCOjTvv;;:LqBOOh        ;U2,,,vAkmCtw.       :Hg0                    
eGe6eGeGe6eGe6eGe6gGeG6GG9gGgGOq@jkwMllvvvvvvvvvvvvvvvvvvvvvv;vvvvvvv;vvvvv;    ..:;v;vvv;vvvvvvvvvvvvvvvvvvv;;vAUkUHkUTXNwvYkjtbCcMATkkmctw; :vHkVvw4kXXUcjcUkUUMLv;,:,;:;;;;;,,,:.;yyVAUtUUwlyVv;;;;;;v;::;;;,,,,,.   ...,::;;;;;;;;;;vvv;vvvvv;;,;;;;;vvvvvvlYvv;;:;;;;;;vvvvvvvvvvvvyLUky;;;;vyhl:vY4ktctctctHccjj0CpX4cZGg6O0#jHUjq0UN4tCOcMv;;,,AZgOOH:      ;0t   ,;vmCjUL      :HZjv                    
6e6eGeGeGeGeGg6g6e6eGgG6G6G69Zpckkwrlvvvvvvvvvvvvlvvvlvvvvvlvvvvvv;vvvvvvvvv:      :;vlYvlvvvvvvvvvvvvvvvvvv;v;vrXNXNkTNATyYvkjtcjcmXHUkkcwv .;MHMvVAU4UUtjqjH4HUkMyv;:;::;;;;::,:::vb0jkHXv .,;,...       . ............,...;;;;;;;;vvvvv;vvvvvvv;:,;;;;vvvvllVvv;;:;;;;;;vvvvvvvvlvlvrXkXky;:;;YyXv;vTUjjptjmtHmmmkmmtkhwtOeGeZDbjHHmbtmNHcjpb2mvv. v00tCCr,    ,vU4    ..v@q0Z;     lbOY                     
eGe6eGeGeGeGe6eGeGeGe6e6e9O@#bjkNALllvlvlvvvlvlvlvvvvvvvvvvvvvvvvvvvvvvvvvlvv.     .;vVlvvvvvvlvvvYvlvlvlvlvv;vvVw4M4UkUrlyyrHpcttpUXNU4H4; ;yrAcTvYrXkNkH2bCtHUkNXArvv;:,;;;;;,::;.lj#Hylv;:,,.    ..,.......,,,.,.,.,.,,:,:;;;;;;;vvvvv;;;;;;;;;;::;;;vvvvlvYllvv;;;;;;;v;vvvvvvvvllAUccUhy;;vYlLyl;vTkmpcjttcmkckmHmHm4UcqZ9OO#CctHmHcmtmtmttj2jHV VbUwmC2v.  ;yMv.       yOO8;     vpc                      
6e6eGe6g6g6e6eGe6eGe6ege9O0CttkXryllvVvVvvvvvvvlvlvlvlvvvvvlvvvvvvvvvvvvvvvv;.     ,;vvlvvvlvllVlVvVlYlVlVYyVyllyXwXwUmcArrMMttjctHHUmkmXr. vNAhkkryvrXXAmj00CUUUUwyVYv;.,:;;;;;:;::vLVVvv;;::,:.....,,,.......,,,.,.,,,:;;;;;;;;;;;;v;v;;;;;;::,:,;:;;vvlvvvvvvvv;;;;;;;v;vvvvvvllyyAwHmmXwY;:VyyYyvvVHccHmHmttmmkcmcHcXUUcHtCODD@Z2kTmHmcjmcmptb0tM,,Vvyk0CpwvlHUM.    .   y0Z9l    .Np                       
e6eGe6e6eGe6eGe6eGe6egBGgD#jchMVYYVvlvlvlvvvlvlvYvYvVlYlVlYvlvvvvvvvvvvvvvvv;.     .;;lyvvlvYvVlYlVlVlVvVVrrMrryhXNwNMXM4NXwh2CjbtmkcHtjX;..vVrTkUkrvlXMAkb@9CHAUkhVyvv::.:;;:;;;,,.vyAv;;;;;:;;;,:,,...,.. ... ..... ..,,;:;;;;;;;;;;;;;;;;;:;;;:;,;;v;vvv;vvv;v;;:;;;;v;vvVlVvlvYyhkkUHU4Ay;vVryLv:;VNHmmUkkcmtHcHcmchMktcmU2O9O99@4AywHcctmtcjtbt4,.,.:AHjjjrVXU;      . ,m22m     #Bv                       
6gGe6eGeGeGeGeGe6e6BeeeeOZ2tAylvvYlyYVYyYyVyYVlVlYlyYVYLVyyyVLyyVylVYyYyyrrrvv:.    ..,vryvllYlVlylylVlVYLyrAArTMUhNTNMhTXXXMppbjjkkkcCCv ;yrwAXhUXrvVhwrHjZZqmccmXwLAyV;:;;:,,:,,,,:vVl;;;;;::;:,,:,,.... ... ... .;v;v;;;;;;;;;;;;;;;;;;;;;;;;;;:;;vvvvvvv;vvv;v;;;;;vvvvvlVlYlYVLMHXUXU4hy;;yyyyl::vlhcHmkkkcmtctttHArjccHm2GOGO9@kyyrtccmmmttmhpjL:  .;TrHcH;;;         ;Ub#k   v8B;                        
e6e6eGeGeGeGeGg6gGe9g66ZbmHwyvYvlYyVryryryLyyYyVyVyVyVyVyVyYyVLVyVyVryLyLyryrrAv;      ,vyyYVlyYyyrVLyyyLyyyTN4yAmkNXwNwXXhrkj2ct4kjCt@y  AUUhXwNMNAlvNTTU2qD##bbckTMrAVv;;;;::,,.,...::;;vvv;;;;::,;,;,;,          vVYvv;;;;;;;;;;;;;;;;;;;;;;;;;;:vvvvvvvvv;vvv;;;;:;;vvvvvlyVyyrywkU4UXkAL;;vVlyVv,;vy4HkHkcHmmtctcthAktHmHc0OZOO9twywcjmcmmcjcmtCUV;;  :vAjpv.          vMZp; ;e8A                          
Ge6gGe6e6e6eG6OGO69OqZpjkNVryryLlyYyyyVLYyYyVyVVYVlylyYyYVVyVyVLYyYyyLVyVryryryVv:     ..yyyVyVyVyyLyLyryLAAHX, ,jHk4UXHtpYY0Cctckpegqk; ;kkhXATAhTXrrrMrUpZZOZZqpkUAXXwYv;vvv;;,.   ;,:,;vyyyv;::,:,,...:,,.,.. . :vlvvv;;;;;;;;;;;;;;v;;;;;;;;;;;;vvvyVVvvvv;v;v;;;;;vvvvvvyyLyyyLwc4UNUXNylvllVlVv;;;;vrkUcHmHcHckccHykmmHmkmjqZ6e9tUVTkHHHkcmtt2pkYv;.  ;jB8N,         .vhCk,le8.                           
eGe6e6e6g6eGeGGO9Gg#0mHUXLrrMrAyyyVyArALryyVyyyVyyyyyyyVyyyyryrVyyyyLyryryrLALyAy;       ;lLryyyyVyVyVrVyAwl;    wtkmkkU2plvOCctjHb#0Xv. vHhrrMATAwMArrryMCZOZOD9ZjhTwH4wyyYvv;;;,. .:;;;;;:;;;;;;;;vvvvvvlvVVVvVYVVrVVlyvvvvv;;;;;;v;v;;;;;;;;;;;;;vvYVVvlvv;v;v;;;;;;;vvvvYYyyywcckwXM4NUXAvlvlvlvv;v;;;lVThHHcHHkmmcUhUmmcHHtqZe6B6Oc4MkkkUHHcmctjwrv:.,.,vkeGv.        ;kcqkky;                             
6eGe6gGeGe6gGeGe6e9CcHMwArrrrMyLVyyArwrArArArArrrArArMLArArArMrMrryrrMrALMAArAyMTV:      ,vTAAyrrALMrArAAcr      4pmkk4UcjYVCCtttHXcHv. ;w4UTArMAMATrAyyVwbOZOZOOOcTyThUrryLYv;vv;.  ;:;::,..,,;;;;;vlvYlVyh4HXUXUMyyylVYryrrTl;;v;;;;;;;;;;;;;;;;;;vvvYllvvvv;;;v;;;;;;vvvlvllNmjjj4MrTrwwkMVvvvvvlvvvvvvvvvVAUHmkmmtcjcmUkUXUjccCe6eG#mUwkUk4cmcmttckw. :v;::COHy.       ;Ce#wv                               
eGe6gGe6gge6eGOZDCt4XwMrMrALArMyLrALMrAAMyArryrLArALMrArArArMrTrMLAAMrArArMrArrLXrl;,    .;vAMTrMrArAAMAk4,      hZmmmckcm4MttcctHmjjv,.vXHcthrAMAwAhAALAAtCDDODO@bkhAhTMAAyyvvvl;:.,,::;::,:,::;;;vvvYvVlVAU4mhHmmhNUUXU4UhUUA;;;;,::;:;:;;;;;;;;;;vvlvlvvvvvv;v;;;;;;vvlYLwrhktkUhwMhTTMwhXLlvvvvvlYlvlvvvvYrUjmcHjt2j2UTMhMUUwymDGOGGgcTNUXXHtcmmcmHMv  ,vvvCqy;       ;w9j;                                 
GeGeGe6eeegBe@tcUUAArwAAAMAMAArArMLArArMrArArMrMrMrMrArMrMrArAAMrArArMATMMAwAMyAAAyV:    . ;rkMTAMAAATrUU;       UbcmcmmHmHk4ctcHmXkkv,vypj0chrTATATAhhNATkbZOZOZZ00mHUUXkwAYvvVVlv;:;:;:;;;::,:,:,,.,:;;;;YYyyLAUX4NcctmHANwUyvvv;:,:,;;;;;;;;;;v;;;vvlvvvvvvvv;v;v;;;VvVyHjjmmUU4UTwAwThMU4AVlvvvvvYvlvVv;vyLktjcjjCpCtUATM44mTwt6G9OGebAXh4XHmcHcHHXwv:   ;;UgjY;    ,VmDZv                                  
eGe6eGeeBeBeeCbckwhMTMTANThThAwATAwAwATMTATAwAwATATrArMrArMLArArMrMrAAwrTAwTTAwAww4wy;.   ..vwUTwATMMAXHL        UbmcHcHHkmkkHZjcHMVy;vhH@DcXrAAwAwMMM4MhA4c@ZO#D@ZZZ#pmmHhyYvvvvvvv;:;,;,:;:,,....   ..,:;vyYVlvvvlyyrrAyylyYv:;;:.,.,,;:;;;;;;;;;;vvvvvvvvvvvvvvvvv;vvlVAHjcjHH4k4NAMLAA4XUAyvvvvvvvlvYlv;vlyAHcckUXHcbcHXNAXUUXHtZOGOebhwUXUUcmmmcmULy;     ;OZk;    ;hGGU                                   
GegeGegeDZZGZ#CjmkwTAhwhAwAhTwAwATAhMwMwAhMwMwATATAwAwrArArArArMrMrTATrTrTMTAhTNTwhNwy;:   ..vwkNNhUXNHA         jpcHmHcHmUkX42DcHryv;YUUC0tMArMMTMhMwMXMTrNc0#O@DZD#ODO@qXyvvvv:;vv;:,:,:,:,:.,.,..   .,:,.;vvv;;:;:;:;,. ....  ..,.,.,,,,,,::;;;;vvvvlvvvvvvvvvvvvvvvyykHtHmmHA4hyrTXcUMTUTAVlvvvvvvvvvvvvvvvvywXXrwkjpqptUMMU4kMhtGG6e#MNNXhkHckcm2kAVV      cDkv   :wOer                                    
e6egB6e0ckcmmUkXHNNThTwAwAhMwMNTNMwAwMwMwThMhMwMhMwMhTwLArMrArMLArTATMXwhMXNXwXXwvv;vvv:.     ;VVlLLrXp;         UCHmkmHcmckmU#ObHTVYvvAHbqmhrArTAhhNTwMMrATHmpZOZDZDZOD9OC4rlyv;;lvv,,,:,:,:,:......         ..,..             ..,.,.,.......:;;:;vvvvvvvvvlvvvvvlvvVMTHmcmmUmXMMMvAUj22TMNTrLlvvvvvvvvvvvvv;;;vvrATXttjjpmmUhw4hXhjGe69cMMUhNhtmmHjjHyyXL     me9l   ye9r                                     
eB6OZ9O0kHmcUhThwXwXTNTXwhAwMwMhMNwhwhMNwNThThThMhTwMhTwAwrAMAAMATTXwXwNXUXkUUrrv:..           .   ,,;;          XqcmmHmHtcjmmZ9cUrVvvy4UqjkrrrwAwMUhXTXAAAhM4U#ZqCqC#q#@D@CmXAlvlvvl;;;;;,:,:,, ... . .                       . ..,.,........,;;;;v;vvvvvvYvYvYvYvlvYVhkkkHXXU4T4AVykmpcwAUArVvvvvvvvvvvv;vvv;;;vvyyyrMhtjjctHmHHU4UZGeDmrwTXAhcckHmckTL4r     X80v ,rB9v                                      
BgOpCCqtcmjcHhXwUwNwNhNwwThwNwXMNwhhXwhMhTNwNMXMNMhTNMXTNThwhMww4wUXUNNhMLrYYv: .                                UOmmHmHcmtmcU#ZcwAvvAcHc#twrlrhH4hw4N4hMrAMhAHcpt2t2bCb0COOO2NLLVvvyyVv;::,:,;.  . .     .                     . ..,.....,...;;;:vvvvvvvvvvlvlvvvvvvvwUk4UUTrhwhAAYwkjtkAh4AVv;;;vvVyNVv;;;;;;;vvvvvvyyh2qCbtmkmHHhU2geGmAMXwMTmkHX4Nmcmry     vB#vvUOOv                                       
CCjHkcHHUUXkh4wUw4hXh4wXhUN4h44UhXwNwXh4w4wNhXhhThMhwXMNTXh4XkUcUXrAlv;v;;,.                   .vALMTMNmVykq0;   kOckHkHHcmtkm#OH4MvvhUHcDmhyVy4mkAhhXwhTTw4wh4HHjjC2bp02@Z9OOhyllvVyLvv:,::,;:, ..... . .                     . ..,.,...,.,.,;;;;;vvvvvvvvlvlvvvyyArNXUUkwhrTATAMLrU2jcNNNUrl;,,;vYrUTTLVvvvvvvvvvvvvvAXpb@0CmHUmHXwC9BGtrTwhAXHmUkkcj02HUy    vOZcpky;                                        
pctHcctHkh4hUN4N4hXhXwNwUXUXkXUkkUkXUhUwXw4hNMhMTMhh4NNwTAwrArLyY;;;;.                          vjCjgBeB888B8G;  h#kUwXhkmtmHbgZcwTvvMcHjqcwAVrhmUwM4whwNhUHHUUUHt02Cb0b0qDDOO@UMv;vryV;;,,,:,:,..... . . .                     ..,.,........,:,;;vvvvvvlvvvvvvvyAUcjUNhUwhrArAyArTkpjtXXUmNAYv,:;vvYVLr4HjcttHVVlv;vvYlrwcCD@0ttctUktOegmArTMTwmkkUjtb@@ccV    wZeB#;                                          
HkkmUmmmUUXUN4N4h4h4h4hUUkkHUHkHUkUkX4h4hXT4AAyrLArMrTAwALlv;;:.                                  ,.  :vvvywrAl  ;UjC2h4ktHmm6O#HhwVvlkq2jmkXU4HUHhwMhMhw4kbbpUUHpjjtCbqC@qD#OOZcA:,vyvvv;:,.,,,...... . . . ....... .     .   ..,.,.,.....,.,,,.,;vvvlYlYvlvvvvyk44h4NUNUNALMrrLTkcb0mrvywAAyv;:;;vvlvyMt@0pCbtcmUkrvvvvlrcZDq0b0pcHtb9OtrMMhAUkkUHmjjD0mXrv. ,be#k:                                           
c4HUkUkNkXUNUw4hUh44k4UXUNkUk4UUHUkUHhXhUhwAMyyyLVylllyYV;;:,..                                               ,  ;km@2hwHHmkH0jmhMUyvyt#ZmmUU4mkkUX4UMwThNHcb2mTkmckmcCpbCZqOCZeBChvvv;vyvv:,.,.,.,.... . . ............. . ....,,,,,,,......,;;;:;vvvVlyYVvvvyrkjcNwrTAwMhrArALrwtcDOpLv;vVXrLv;;;;;vvlMH@p2tbjckmc4lVlYvlrjbCjCbCHHcb@6CkMXMhkmHmkmHpZ9cXLVvyU8Bb,                                            
cHkHkmkk4k4kUUUkUUUHUU4mXHHHkcHHkHUckUhUNXMhryYyVYvv;v;:..                                                    .vvvyUpbT4kmkHUtkULwNrYAcZ@HNUhHcmUUhjcTyrAUktC@mXUcccmttCjCqDZC0ee9CkrLvvvvvv;,...,.... . ......,.....:,,.,.,...,,:,:,:,,,,...,:;;vvvvvlVlVvlYMHjkkhUUwyTrAyLrALryXcj@OcAv;;r4wl;;v;;,vvyYwHCO#tt4k4HUAvvvvvVXckct0CbjCbZG#UXTTAmHmkHUccbq@tklv;.l9j                                             
4wkUkX4h4X4h4wUhUX4Uk4UUmmjtjjbtjmtccUUh4wXMrvvvvv;;;;;                                                    .;;;vl,vNcXN4cHmUUccckAhMryUp#4MAkj#Hkkk2jMVvXkctqqjwUtpcttptpC#@ZbZGeDDC@2cl;vvVv,,.,.,.... ......,.,.,,;,:,:,:,,,:,;::.:,;,,,,..,;;vvlvlvYvyllrkcCCkTUX4rTrArryryLyXHjcqml;;;vAmTl,;;;::;vlvvAteG9CHkHkcAyvv:vlXH4Uj0#Z#C009ZjThAAUmUUkcmmHjjbHVv. ;Zw                                             
wAAMrArAyryryAVVvVlMN4w4XUhkkccpt2ppmHhNhNAAlvvvvv;;;;;                          .                        .vVLVv:.lHmMVTUHUcc#qZ0pH4AXMyHkAUm@ZjHct@twvlhmHj2qbchUcbjjctmjjCCbbD9OD9O69#yvvvv;.,.,,,.. . ......,,,,:,:,,,,,,,;;vvv;;;;;;;;,;;v;;vllYvlvYrMMcccMArMTNATAMAAyryVlyHjcttX;::vrkHy;,;yv;;vvvvvvm@9OckmkmcHMrv;;lMkkmHbC#0#b0COC4AAAkhXwcttcccptjUyv yG;                                             
YvMhhwhMArAyAVv;;;vvryArMrrTkkct2pbtjHH4kwNryvlvlvv;;vv                         ,,,.,........     . ..;:,;Nr;:v;.,vAmHUvVXUkc2@#Z0jUhM4yAHmcjjbmccpbjMllNkmcCbqpcwkcptjmccjbqpjCODODOOeeOUv;;;;...,..   . ......,.,,,,,,,,,.,:;;vv;;;:;;;::,;v;;vvvvYllYAhUHtNrVYvlvVLNXNLryrVTkjcjjkv;;vTpCcrv;vvVvvvlvvvvLct2kmUHktkMVVv;vvvkmHkmtqCCb00CNAyTNXwTktmckctjc@NYv:A;                                             
vvlLVryyYVVyvv;;;;;vvlvllyVAwUHppbbjtjmcUNTArrllvlvv;v;                    ;:;:;;v;;,.:;;;v;;vYrvvvYyMhNTkmNv:,:,;:vv4th;ykHhktCCq2pUXTXrkttcjmcmtcchAvAkcmttqbCcUMcj2bCpbp0C2bZOG9OZGGeg90mMVv;   ... ..,.,.,.,,:,:,,,:,,,:;;;;;v;v;;;;;;;;;;vYvlvvvlVwXtjC2tmkyylYvlVNhryrMXH2pbcHrv;lMUt#jHyvvvvvvlvVllvYLXUHHHkcmNLATA;;,;ApmHkcC#C0b#ZqcwyXkHAXUXhhXtctpcrMvL;                                             
yvlvvvvvv;v;vvv;v;;;;;;;vvvvyAUtCjCjpctmkhNMMyYvvvlv;;v                   :vv;;;vvv;;;vvvlyvlMtpcmHUUMwMAyArl,.,v;vv;vMjXMHwkkpbC00CjUhNUUcctcjcttc4hLyhjctctjCjcNMcZ0###C0b0j#OGOOO99eGGGggepkY;.    .,::;:;:;:;:;::,;;;;;;;;;;;;vvvvv;;;v;v;llllVlYvNpCckXUhHwVvlvVVVrhYyyXHHc0qjLv;vNjcNTUNAVylVlVlVlYvvlAw4HtmcmckmmcAv,:;wctmHkjq#CCCOOZhlrjchrNXUhkmtmcccUy                                               
v;vVlVlvvvvvvv::;;:;,:,;;;;;vlYUcjjbt2ttUhANTMlvvvvv;v;               .,,:;;;;:;;vvVyyvvvTVvVUjbcHyv;:.,.:vv:, ;;;;v;vUCkUrTUc#Z200CmkAXUHmtcjcjt2cmhMYNctcmHtcjmUhtZD#D@#0CpCCOZDZG66GGOG9eB8GDkAv:.. ,,:,::::;:;:;:;;vvvvY;;;;;vv;;vvvvvvlllrwlyyAMNUjcNvvvlVVvllYlVYylYYMwHkctOpA,;wmmkArLMAAVyyylYvYYyVylllyMcHmcjtptky;.;vMHjHkHj0Z0q@Z2kyMmcMwhUXhMHccmcmch;                                              
;:vvvvvvv;vv;:,,,,,,:,::,,;,;;vYUmp20pbcmUkU4TVvlvlv;;v              ,;;;;;;;;;;vyh4j2ckmcUXHmchMyY;: ..,,;;;;;;;;v;;;mq0mcMhm9OCbbctkUUmHtcjmtmccmkHNwwmmtjjHcmcHtj@CZ0qb0CC0Z#Zq@D6GgO9OGG668eZjhvv,..:,;:;;;;;;vvvvVlVyrYlvvvYVVvYvVVrYrrwrUHhrr4tckLrVVvyYVlVvYvYvVllvrrXUHtq2Hy,lqUTrhwMwhvyrYVrllyrwHAVvvvAwNUmttcpcHV;;;ltbc4Hmp0#0ZZDDpANkUXkNUXXkcmcmtckr.                                             
;;;v;;;;;::;,,,:,:,:;;;:::,::;vvyUc0bCjtcjtm4hlvvllv;vv;..          .;;;,::vvlyUc20G9ZjCCCtpHULlvllV;..:,;;;;;;;;;vv;vLCO8B#mmhC9ZCCtcHccccpcjHmkmkmHmHH4ctZCcmtmcUmc2jCCq000@qZq#0DOGG69G9eGg6egBZ2kAv;lVvvvvvlvyA4wXhryAryyryTwUNhwNwUhhNtmmmtmUwjjULv;vvVlyVylYllvvlllhXUkHUmmAvlY@ZyvrwjmmwvTmyMAAVyYhHUylvVLMhcmcmcmttcl;;Lt#chhHt0@DqZZ9CUTUUmUHXUwmccHmctUV                                              
v:;;;;;;;:,:;,..,...:,,,:::,;;;vVhtb@pjmtttUHUyvlvYvvvv;,..       . .:;:;:;lXUp#OZO9eOOO6gOCkyVvllyyV;,,::;;;;;;;;;;vYrwbBBOCHNkOOZCjmtmtmttjctkHkHkmkHUkmb0@jccjcHUct0@DZOqZ@D#Zq@ZOOG9gG69eGe6BeBe9#pcqcAVrrAMhyVyyVrVAMTrrATrhwhMhTX4cHmcpcjcpjtUrvlvYvlvllYYVlVlYYylAHcmmkHUUyvvtet:;VXcttUv4tTTALyVyTCmXryyLVTUmHcctcjpmY;lkctkkmjCZ@D@ZOqNNNU4UkHkUktHcmtmchv                                             
;;;;;;;;;;.:;;     ... ..,.,,;;vyNkCq@2jtCtmh4yYvlvvvv;;,,.,.,...,,,,v;;;;vLhtCO#ZqOD9ZC2cwyv;;;;vvvvv;:,..:;;:v;v;vvVMNCBODCtUmc#pccjctHccpjjcmkHkHkmkH4Hm0q@jtctHkUcCZZODDZZqZ@D0ZZG9GGgG6Gg6gGeee6egB8gcmHtmttcwNwhUHmcHUTNAhM4NUUmHccjtttpjjcHwyvllVlVvlvYllvVVyYyrLTmkmUk4mHhYY#BA;;rhjpCUVTHrhrrVAUyLHcHMMVVvrwUUjtpt22CUlvlT2pjcC@O#Z#9CwrXTXXk4HUmmcmcmjmkA,                                            
;,;;;;;:;;,....     ........,:;vrAUt##9#0qZ2cwMrVYlvl;;;;,,.:,:.,;;;;;v;v;;vwkj@OZOOeGcv;.    ..,,:,:,;;;, ,;,:;vvv;vvUcBBDC0HHjpppHtcccmcttbpjmcHmHmHmkHXkc0qqCjcmkkkHtODZ#9ZOqZq#C#96GeGgGgGeGe6e6egBeBeO#Zqq00pbHHmpjcUk4HkcHcHtcjjC2pccm20jMMyVlyVyVyYlvYvlvVvyVylrr4kmUUhUkmTvTZmvvyAc@DqcMhhXhUryH#VvApcrAHkhlXhXUpjppC2CMvvYUbtppqC#qZD#4AyThXhUhUHtHcHcttky                                             
;:;;;;,:::......   ..... . .,;;vvvrkm#ZDqD002cMkrXThLv;;:,.,:;;;;vvvvv;v;;;YhmpOOGO@pA.          ......;:: .,, ;:;;;;;LUe8BBe6D@jpttctccHtcjj2ttHmkmkmkHkk4mb##D2ckHUHXHZOZOZOZZ#Z@#0Zq#Z996GgGgGeGeGe6geeOqbq0@jptbjC2btjt2jjtjctttc2b0p2cmHNvvvVlVlVYyYVlYvlvYllYyvVywhUkkUkUmUyvUCHvvYXcOZDmwhkUHUMVc0yvhpmYNOe@H4rvyTctCjCpcVvvLHb2btCbq0Z0bMyrXwXMXTHHmmcHmcUv                                             
v;vv;::,;,, ... . . ........,:;;;vyhHpOZZ@0bjhyMAMUjjTv,:...;;v;vvvvvvvvlvvyXUpDGqcrv.                      .  .:,,.,.vytOeB8BBqqb@0Cccmccjtbj2mmmcHmHmkHUHmC@D#0mHUHUUkZZDZOZO@##Z@@tjtC#OOGGg6g6g6eG6G6GGCC20CCctj0pCj2t2jppCppttj2cmXUTwlvvv:;;vvlvvvvlVvYvlvYlVYVvrrTNmkcmmUUYycpAllywt@D#thkUU4krymZvvAbwlw9GGqcVvvrMmjbtbCcV;vhtptb2CCq0ZbhyXXhATMhUmHmHcHjw;                                             
vvvvv;;;;;:,.,.............,,;,;;vvTwt@Zq#0#mhy4TAXZgDAv;;;;;v;v;vvvvvvVlVlANNwkkUl;:. .         .......               . ;vvvvvwH0@ZqCbCccc2jC2tHcmcHHkHUHHtbZqDqCkUkk4k@O#Z#Z@OZDDDq2jC0OD9G666GgGg9eGeGe90b#Cqjjtjj2p2t2jCttcccjcUwhyVvvv;vvvvvvvvvvvvvYlVYVvYlVlyVyrMAXHcbDkhArrkNMrwTTActC2jkkXUwwrCDyvMkAl4qOO90cHmXrrctccCOtlvlMwtpqCqCCq@wAkt4ry4kcmcHHc@qX:                                             
Vvylv;v;vvv;,.. ..... ....,,:,:;;vVyNH@#Zq#@mrAUHNpZ8Bcvv;;;;;;;v;vvvvvlVYyLrlv;:..                                       .    ,;VktC@Z0CtttpjCmmmcmmkmHHUmc00ZCO@mAHHkkDOOZO#OZOZOODjjtCCD#G9gG6GgGeGgGegOp@0#CCtjtbjpjbjbjtwhrrMwvvvv;v;v;v;vvvvvvvvlvlvYYVYVVyVyVyyAANNtpZ2UrAAhAhh4h4MwkmHcmmUUhNAHZDyvrHryX@#OOOjjm4yLNtmHmZ2hlYyMkCCqC0j@ZmAmjtyVMtHcmckjCcAr;                                            
ryrrVVvvvvvv;:::,,,:,:,:::,::;:;;vvAMXpO@Z@Z4VLmjOOee8Hlvv;;;v;vvv;v;v;vvvvv;:.                                                  .;TcDZZCCtjtpjtHcHcHHHHkUNHcC2CCqcmHcHmctCODD#DZO#O@bmHHtj0#GggGeG6969gGe#ZC@q0jjj2jCpC22mkAVlYvlvvvvvvvVYyYVrMYVVALVlVlVlVYVlyVLyLyrANwm2DZ2wArMMhMhThMwAUHcHmH4w4MwHDbNVrMMyNCZCOObmmhAVVrmmmpO2rlLyHtb2qbCC6CkhtcAlMHcHtmctCLypU.                                           
rlrVyyylllVvv;v;;;;;;;;;;:;;;;;;vvvlAAcZD0ZZcytZG6eGBDrvvvvvvvvvvvvvv;;,;::,:..   . .                                              ,VU##ZC0tbtpctcjcmkmmmTXUtjjtjcbp2ccmckmmpqZqZ#ZbbtcHccCCZ9e9gGgO999OD0pcpcmpCjCpbjtcjjmMMMhwAlyYylYlVlVYVYyrrVrkkyVlVlVVVvYvlvlVyL4UAXp0ZpXrArTTTMhhHUNwHHcmmMww4hmttkUhhTMUC2bC@jtccckLLktmjZcVYlwHtc2tCpbO9tXLrAhkcmcmmH0tAymr,                                           
yVyLVyllvllv;vv;;;;;;;;;;;:;;;;;;vvlVwHZ###DcrH#6BGeeOyvv;;vvvvv;;;;;;,.,, .       .                                         .     .;Amt0Zbtcptbp0bjmmHckXhmc2cjtjpbtcmtmtALAmHj2@C0jpkHHjp0#GGG9GO99eGDCj4UUUwmHHUUhXMAAk4wTUNXryVLyyYVvYvYvlYryyyALyvlvlvvvlvvvvvYVr4HMHtC2bmhVyrUwANccpmcmmHtHhAAwHHcHHNU4UwcjptpcttppC2thNTAMjpHv;yUUmmtcpcCZDAvyHm2tmkcmtp4vvAT;                                           
AlryLVyvlllvvvV;v;vvv;;;;;;;;;;;vvvvVrU2qpqjjUctOeBGe9Mv::;,;::,,,....... .                                                      .,:vvvr2ctcttjjbbqtkkmHmUkUccjc2tpjjmccjtwLThkHjpqb2jtmpp0q#D69GG6Gg69tckTyryrrAyVYlvYvyrhTXh4wAVyyyVyYVvllllyVyVVvvvlvvvvvvvvvvvyrNwHUHHctjcjwYlAU4rNmjjpccmmHkTAA4HckmUXAhNNHjctt2jbp2tCtUrLyAUpbwvyMUkcmttccZOrvTkjpjmcHHmUvvvvv.                                           
ryrryyyyVyYVVVllvv;vvv;;;;;;;;;;;vlVYyAtttmtcjtmte6geGT;.;;,.,.,.,...,.... .                                                     ..:;;;vU2cccjtjt2bpUUUHHmHmHmmjcttpjjcjtbkTMUHmmtb#0qp22CCZqD96969geeDmXUAlvyVVlvvvvvvvvvyk44hhryYyVryVlyYVYVllvvvv;vvvvlvvvvvvvyrkUmmmUHmcmtjkVVrMMMTHjbtcHcUH4UTNkcmcmckHUHkmccccccctcttpUNMwMXtDbXlwUmmtcpcpDGcALhNUXmHXry;;VL                                              
AYryyyyVylVvVYYlv;vvv;;;;;;;;;;;vvVVYyNcq2jCbHcXmgeOe9w;:,:.,.,,,.,.,.... .                         ,:;,...                        .::;;yj0jtcjcjtbtHNkUmHcmcHccttttjjpjppthwTXUkU200bCj0b#q@CODGOG6g69cHXwyrLLVVvVvYllvvvMbbppHhyryVlVvyyyVVYVllvvvvvvvvvvvvvvvyMUkcctckUmcptqkyyArhAXktCqjtXUhUX4kcctmtctttcjmcHmHckcctctHUwXwwMjZqTlVNmjtjUUHqOphArryAUtNMV;;rA.                                             
rVyyVyYLYyvlvYlvvvvvvv;;;;;;;;;;vllyllL4j2pqkMUHmeGO9qkUv:.,,,.,,:..                     . .....,;;vvyyylvvvvvv;.....            ..,,;:;Yj09bcmjmttjkU4kkckcmmHccccjcjtptCchAwTkUmt@C0C@0#0qC@Cq#ZD6G6#tN4AyyryVlVYyVylyllYMyAMhryllvvvvvylyVYvvvvvvvvvlvlvvvvvYrUcbtjttmttCCqCNlrrwMwwmt#Z0thNHUkUmcjctmjctmtmmHmHmHcmtccUUhXw4w4cCtUYlycc@HyyNmZt4AhhMMHtkyv;lAk,                                             
AlLyylyVyYYvlvYvvvv;v;;;;;;;;;;;vlylYvLXpjbbHrTrpe6OOcmUV,..,.,,:,,                  :vvvvvvv,   .:,;;;;;;;:;:;:;;;;v;;:;:,.... .,;,;:;;vvwCDZCttcjmtccUkHmHcccmcmtmjctcjjmAArhUccpC0CqCq0@0@C0pOGG969OmAXTYyyrlyVyYyVyYVvlvvvylvvvvlvvvlvlvlvYvvvvvvvvvvllvvvlVTUbbpCcmpqD@D0D2chAyyrhXHjOCjMXUUkmHtcjcjttcjccmmUkHcctcccHhUwUXUkmHmAlvyUpbjTrrTUmkH4UUHUmHy  vUk:                                             
yVyLVyyyYYvVlVvvvv;v;;;;;;;;;;;;vVlvvVrcbbtCmAVytGD9qcUNv, ..,,:,;,.                .vlYYYvv:        .,:,,,,.. ,,;:;;;:;;;;;;;,:,;vv;v;;;vyjGg@bcttjtpcH4HHcHcmcmmHmmcctcpHUTMAUkmt0bbpC2qC0bqCC#9O9O6OjAwyllyVYlVVyYyYyYYvllYLLllvlvlvYvlvvvvvvvvvvvvvvvVvlvvvVyXjZ#OtkcZZO@ZOeOpwVvrTNyNqbmM4cHmmtcjttcjjpjCppmHAXNHkmmcXNh4XUUHkkUyvvvhccXMlVlLXchwwUhMXtv  .v;                                              
AlLyLVyVyvYlVllvv;vvv;v;;;;;v;v;vvvvvlrTmmj2UvvvkCZDO2ptr;,.,,,,::,       .         :vvlyYvv:         :;;,,.. ....    ,,,.,,;;vvvvYvlvv;;vlU69#pjt2cjcjHHkmkcmcHcHcHcctctt2pjwAhmHpbCppjC0ZqZq#bZOGZO#ZCqcHMX4kAYYyVyyyVylVllvLLLVyvlvvvlvYvlvvvvvvvvvvvvvvvvvvvLTtqOOthtZO#ZqZOGCcNMANTAw@jchkmcHmmtcttjjCpb2CjtkwrMhHUmHTrNXHUk4U4UAYvvAUMylvlMAmcHUkUkANM;                                                   
ryyrVyVVYYvYlYvvvvvv;;;;;;;;;;;vvvvvvyyvVXcC4yYrH0qDZ@20Uv.,,:,,.,..             ,::;;;;;;..          ,:,,.,....      ,,.. ..;;vvvvvvv;v;;.:4@DZppcHkmmckmkHkHHmmtmcmtmctjjCjUrhkctbt2tpb@#Z@#qqqODODD@@pCCmXUmkyyyAyryLVVYVllVrVlvlvvvvvlvvvlvvvvvvvvvvvvvvvlvrhcjOG9khcOOODOZO9ZjcwwAhrX2jkcHcHcmcmcmcc22CpCpbmcNwrkHmkckhLXHcUUUUXhlvlMAAlv;VMHcbjcc2mUXV                                                    
rvyVylVVylVlYllvv;v;;;;;;;;;;;;vvvvvvYylVrcjHyyrcC#@OpmmX;  ,,,...,...           :v;;,;  .            ..,.. . ...     :,,.. .,:;;;:;vvvvlv, ;r#Z#2cU4ktmcHmHmUHHcmcmcmcmtcjt2kXhHHcjjcjtCC#@ZqqCZOGD9OOCC2jhTrXrLyryryLyyYVlYlyVVvlvvvlvVlVlYvvvvvv;vvv;vvvvvvyhCC0COCUMjOe9GOGOObbcHTTAMLhUmbjHcccHHkcHtjCCCpbjtkkwMXmUcmt4yLUHtUU4krlvyLryy;:,:;;;;:vyAY;                                                     
LVVyVVlyYYvYvlvvvv;v;;;;;;;v;v;vvvvvlyVyrHt2UyYNpO#q@jhhV;  .:,.......       ..  ,;:,,.               .:., .   .      .,..       ..,;vvVlLv,.;vcqqcHXkHcHcHmUHUmmtctmcmtcjcjccHcUkHptpjpjCbZ@D##@GOOZO@#CZjHTAvVyryyYlvYvlvvvvvyYlv;:;;;;v;v;v;vvvvv;vvv;v;vvvycZD0bjCH4k0O69G9OD@tjUryArrNHHCjmUcmmHcHcmjp0b0bbcmUUhU4UkkXNyMXHkkUkkAvYyLyl;,                                                                  
AlyVyVylVlVvlvv;v;v;v;;;v;vvvvvvYvVyrrArUmtttMrtZ#Z@ZCpm4;. ..,.. . .      ,, ,..,;v,       .         ..,.,....       . . ... . ..,.,.;;vvv,:,:vMXcmjjbj2tjjttjccctctccmtttctmmkmkHmtjCj2jq0Dq@2tHt2#@ZC##ZCt4MyrVyllvlvvvvvv;vvYvvv;:::;:;:;:;;;;vvv;vvvvv;lyTcO@#20@phXU@ZODGOO#0cNVyyylAAH2CcHHtmmHmkmmCC0C0qCcmwMrwhhwhTUNXhXNUwrvllyyV;:.                                                                  
yVyrVyVyVVvlvv;;;;;;;;;vvvvvvVlVvYYyVylrAh4HHmrU0#qZ0Z0bmr;,   ... .     ..v, . .:vDv .v   .          .,,,,,....           ........,.,.;;;;;;;vv;;ycqO@#0#0qqZqCmmmcmmmcHcmcHcmcmcHcHtj2jpb#0@2jHUXcC#Cq0#qOO0mXllYYvYlVvv;v;v;vvvllv;;;:;:;:;;;;;;vvv;vvv;;vkmb##0@qG@mMyTpbD9Op2jtHmUTvvvAUb2jmtctctmtcj2qCqbZbtHrYYlAwUhNNUN4wUNMvvlyVY;;;,                                                                  
LlyVyyyVyYYvv;v;v;;;;;;;v;vvvvlvlvYlVvlvyrHHNTXmq#Z#ZC#btNTr;.... ... . ..,     vl,    .   .        ,.,.,,:....             .........:VyrrwXUXrvvvYk0C#C@CqCCCqpcUHUmHmHmHmkHmcctctmmHtcjctjCjjtcUtCOq0C0C###ckArVMryVylv;v;vvvvvlVvv;;:;;;,;:;;;;;vvvv;v;;,M2p2Z#Z0@gebcNkt@#eOpc2jC0jwvvyTmtCjtcjcjcjcpcbp02CqCkUyYVyrUHkwNwUwwhXylvvYyv;::                                                                   
yVVyVLVyYVllvv;;;v;;;;;vvvvvvVvYvllVYVVYYAwkLAk#0ZZD@Z#ZbmkmAv;;:. .     ,.    .jL                 ,,:,:,:,. ... .          .. ..,:;vw@Cb0btckryAhhjC0p0bCC0b#CCHUUHkHkHHmHmHmHjcjcckHmtctcp2bttkcpDOO0@pCbqbCb2HUXUyyVYvvvvvvvvvYvv;;:;:;:;;;;;;;;v;;;;vvYApOjp@Z@#@e6OCpct2Z9Otjt2jbmUvvVAXt2btjjtcttjjbC0202ODHVvvw4kHcUXThTwMXwAvvYyyv,.                                                                    
rYLyyYyYyllvv;v;v;v;v;vvlvvvYVyYVlyVyVyyyyryrhHqO@Z##ZO#Opmmthlvv,   .  :v,    vM     .:    ,.,,,.,,,,,,,,,.. ..,,,.,.           .vAhmjhNXhv;:.;jCjtjtC2bpCC#0ZbHXHkmHmkHHmHcHtcjctHkUcmccctbtjcttq#9DZC0C0bqC@jUANhN4wVyyV;vvvvlvlv;;::;;;;;;;;;:;;;;;;vykHCCjH#OO#DD9O99DcjtjcccttpjCjY;vlrUccjtjcjbCb#0@CqqZOGmV;vMtHtHHhNThAwMTyyllvv;.                                                                     
yVyyVyYVVVvvv;;;;v;v;vvvvlvvlyYylVVLVyyMyAyyywmDOZ@OZ#ZZqD0Z@bUrv;,.,wV::Y.           vmv  ;;,,,,:,;::,:,,,,,:,:.                .,,       .:;:AZG@@tCC0bCCqq#qCkUrwXXhNNHmmkcmtctcH4mkmHmmtctccmjb#ZOq@CC20pCCbccmkXUMVVrlvvvvvvvvvv;:::::;:;:;;;;;;;;;vTHmHHkkcptjcCCOOGbjcjttmtcjtj0qA;vVyrMttpttj#Z#qqCqC#Z#kyvyAcccHmXUwXMrlYYVvv:,.,                                                                      
rlLVyVVYVvvvv;;;;;vvv;;vlvvvVlylVvyyyYyLryryyvAC6ZZ@ZZD0D#OZD#0mAvvvlHT.    ;y   vrvmv,rkvvvv;;;;,;,;;:.,Vr:,,::                ...     . . .,:VpjCtUcZC#C0C00qCqjpHUAMMUkcmcmccjctmcHcmtHtctcjcttC#OZ#q#C#0@C@0Z@tNXwAvYlyVVvvvvvvvvvv;;;;;;;;;;;;;;;;;vAmHHUHHcctmctj2#Cbjbjjcpctcjmb0U:vvlA4p2tjc2qDq#bCCq#@HLvAktjtHHUXwXwTrVlVll;;.:.                                                                      
LyyVYVYYvvvvvv;;;v;vvv;vlVlllyyyVVVyVyYyLALrrYyq9O##q##ZZDZZ#DqOODULAHM;   rHr:  :Umv:yUwHNyrtwrl,.,.,                        ...... ,;v;v;::vVTjODmvMtccjjCCZC0C02cXMMNUcmcHmmtcjctccctmttjcjtjtbqODO0#0#@#q#@D##CjUkXTyAyylYvlvvvvvllVYyvv;;;;;;;;;;:vvwkcUHUmcjjtt2jCp0b0bCt2cjctctpDH;lvv4HCbCpbbq@Zq0jCCOjy;VkC@ZtHMhAArhMwrylVVl;;,.                                                                      
rVyYVYVlvvvvv;;;v;vvvvvvVlYvVVLYyVyVyVVrArAyLlVMZ99Z#@D#DZD#D#ZZO90HmmjkY,,v  ;lv    ;Avvrkyl;v.                          . ... .   .,;;;;;vvvv;rmCHyAXrTTmjC2@C@b#btkUXmmtHcHcccmtctcjt2tpcjctctjZZO#ZqZ#Zq#qZ#OZD#0mHHkArVVvlvvvvvvvlvlvlv;;;;;;;;;;vYrrkHcUMAHkkkccpp#0q0qCbtptptt2OGj;rvrUjCqbqbC0ZZDqC2CbcvvT2DO@2UXrArTAMAwrVvv;;.                                                                        
ryyVlVYVvvvv;v;v;vvvvvvvllvYlyVylyyyVyYArArAVyvrtODO@#qZ@Z@Z#Z0#@OCCjjqZUvvt: ;y,    ryv;vlv                               ......,.,,;:::;vhyv,  lNUAryArhkbp2C0C@0qbbmwAmctctccmccccm4Nktcjctt2j#qZ##qZqZq@qZZ#tpq#jmkmUTVyVlvlvvvlvlvYvvvv;;;;;;;;:;;YyAhUTMyrMXhUUkkmt2tjctcjt2tjc#Og2vvlVhcZ2CCCbqqDZOqqUMrLAt#9@bcHhrrMrMAwMMYvv;;,                                                                        
rVylVlVvvvvvv;;;v;vvvvvlyVyYyVyVyVyVyVyVyVyyyYVrcZO@Z@DZD#DZ#@ZqZ#Z#ZqqqO2cpy;Mv;;..  :ll;                              ... .   ..,,:;YvrLyYVvvv;.,VCHArTA4Umcpb0C#C#DpALTpctctmtmtctUrvVwjpb2@CZ##q@C#@Z@Z0ZO9mlNmmjctkkwAyylyYVYVvVlVlYvvvv;;;;;;;;;vlrLMLrVLrTAwwXTwrwTwTXMkj2t2tp0OOCVvVyAt0qbqb0CZ@ZZOpUYhtq#ObHXkkUMhwwAMMTyLvv;;.                                                                        
LyYYvVlvvvvvvv;;;vvlvvvVVyVyVyYyVyyyVyVyVyVyyyvrtO##@D#OZDqZ@Z#Z#Z@DZOZOOGZmvlUHvYmw   v,                             .,,,..  ..;v;;;wpcwwyvvllyv,.lHG9pNwATrANjb@C#C0mNyNmtctccctccHHHhvVwCbC000#0Zq@0#qZ@Z@OOcy44ctCjHhXArVVlylyVylVlVllvv;;;;;;:;;vvYyTrVvllVVVVyLLyyyMTUN4XtcjcjtbC#bmX4MXcDC@b#Cq0OZZ0cwUm##0cHTXUk4kUHUkUXrLVVv;.                                                                         
rlylYYYvv;vvv;v;v;vvvvvlrVyyLVyVyVyyLyyyyVyVylvYjZZ@DqZZZZZ#Z@ZqZ#Z#Z@ZqZ0Be2Cjr;yHY;:,                         ,...,..       . 4ZbkUe8BCcM;vvvvrv,:Vq92mrMAMLwt0q@CbmcwLAmHmHmHmkmkHHcHr;vwjcjmmmq##0Z0ZqDZDZDqqjyvmjjHkNhTMVYlVlYYYlVvlvvv;:;;;;;;vvlyTrYvv;vvllVYVlVvyAUUUhXHcctjCb0j2mkNXkCq#C@00@OZOO0UmttUHXMAXHcHHhUUUNXryyMArv;                                                                         
yVllvYvv;vvv;vvv;v;v;vvlYyyLyyyyVyVryyVyVyYylVvYh2C##Z@O#D#Z@ZqZq#@ZqZ@ZCZZ99B60jGb4XL.  . .                   ,,:,,  .:,;vAwkhMjGCCb9O9#e0ytv lOt:.vwHjpbj2H4XHc#qqCDZZjHUHkkUHUU4kHcmpcA;;vUmjm20Z##@Z@Z@Z#DZOZGk;yjtpccHHhUwhVVlylYvvvvvlvvvv;;;vvYVrv;,;;;;v;vvvvYvVyNNkUUNmmcmtj2jpmmUU4c2qCqCqCq#qbDpkmZCkMXNmH0ZD@CccUUrryAwHUNryYVv;:                                                                   
LlVvvvvvv;vvv;v;vvv;vvvvVYyyLYLyyyLyyYyVyVVVylllwc@0ZqZ@ZqD#Z@ZqZ#Z#Z#DqZ#Zq#ZOZ9OOOCLv;v.  .       . ...   . ... .  .AjbC9e88BGBBGOg96O9G9#6; yOpyvVAXc@0@CDObUmb#2CqZ0CHUXkhXhUUkHcmtb@cy;;vHC@@q2q@Z@D@O#DZOD9gZyActmpjtmm44MrlYYylvvvvvvlvlvv;;;vvvv;:::;,;,::;;vYylyANUk44HcHcmtctcjctctmttqp2ptHtHHkk4tCqtjcpC0CZ0ZZ@tcmjkkhUhhrXcqUV:.                                                                   
yYvvvlvv;vvv;v;vvvvvvvvllyyyyLVyVyVLVylyVyYyVYvlhb0#0Z0Z@ZqZ@Z@DqZqZ@Z@Z#ZqZ0Zq#qq0OpkAYvv.         .,.,.  ...    ,vvkOBB8eBeB6e6B68B#mCmhjejv;mBpvyTXrkjtmUbGtHktbptttjmmkHUUwXNkHcmtcb00tr:;Mb#00qq#@O@D#Z#OZOZG0rrkY;H0ttctkkTrVMVlvvvvvvvlvvvv;v;vvv;;:;:;:;,,,;;vvvvyrXUkUmcjmmctctmcccctmtcptjcmUHkHUHtCCZ#ZqOq#CZC#0qCqq0tjkXrl;v;, .                                                                    
ylYvvvv;v;v;v;v;vvvvvvlvvvlvlvYlYYLVryLyryyVyYvvXpZq#CqqZ@#0Z@Z@ZqZqZ#Zq#q#q#0Z0#q@qZ#qhMrv,  ...   ..,.. ...,v;YyHqZ9Bg6G6Ge999669Ccy2Om:LCpHpG8v v2kv;v;vlH2cmtcjtjttttmmkmUUwUkjtjtbpCbZtAvvYHcp0Z0##DZD#OZOZO9Cyymy.UCtppctHmhAyyvvvvvlvlvlvlvvvvvvv;;;:;:;:::::;;vvvYrArVLywMkttmcmmHtctmcHcmcmcmtmtc2tCC#D9DOO9Z#@Z0ZqZZ9Z@0m4X;                                                                          
lvvvvvvv;;;v;v;v;;;;;;;vvvvvvvvvvvvyVyyyyryryVvlACqZC0C@Cq0#@ZqZq#@ZqZ#ZqZqZ0ZqZqZ#D#OO9#jHl;;,,,;;::;,,.. ;vMUjO96BeeG6OGOGO999O6G#lvUXLrvHtVyjA  c#V:::vANUchmHmUtctmjccHcHHwUNmjjtbbbpbjjckv;lUmC#Z0#ZOZDZOZODGZmL4hlyHUjjbjHwAYVVVvlvlvlvlvYvvvvvv;v;;;;;;;;;;vv;v;vvVrhvv;vVywtctctHccjcpttHmHmHcc2tbpqqZZOOOOGD9DZ#Z#DZODOC#tj4:                                                                          
yvlvvvv;v;v;v;;;;:,.,.,,;;;;;;;;v;v;vvv;v;vvYVYvYNbC@Cq0#Cq0ZqD###Z@Z#ZqZ##0#q#qZ@DqZZODOZ#tHM; .,;;;;,.,. 48G6OeeeG69699OG999G96GB0vvkhwHrvvYUjp .yv :LwVAwArU4hMNkcHmHmUmkk4UAyUCcjjbjCjcH2j4rhhmpZ@#@OZDZOZOZ9OOtHH0#H;:;c2bkNryyryVvYvYllvlvYvlvv;;;;;;;;;;;;;vvv;vvVYTAYvlVLyTAUkmmtmtjptjcmmcmtcjppjb0@0ZZOO9OOOOZO#DZZqD@22CZm:                                                                          
lvvvvvvv;;;v;;,,,,,:::,:,:;;:;;;;;;;;;;;;;;;vylvvwt@CqC@Cq0ZqZ0Zq#0Z@Z@#q@0Zq#qZq@0#C#qZZD#0C@cUVvvv;vvYvvL98Bge9G9GOGOG9GOG999G9geZv;YAlHA:,vj0pjv  ,vHcNlVyXUUN4UmkHUkUU4kUH4rvHpjtbCC22cct2jjAAUtbD#ZqDZZ#ODOZ9O#MVk9Dh,.vUtmHcNArMrryLyylVvlvlvvvv;;;;;;;;;;;;;;;;;YVy4MvVlyyrrNXmHtmmkcHtctmtcpbCbCppbZ0ZqODGDODOZZZZZDq#ZDZZcpV                                                                           
Yvv;vvv;v;;;;;,.,.,,:,::::;;;;;;;;;;;;v;;;;;vvlvyUCq@C0bqb#qZq#q#0#qZqZ@ZqZ#@0#qZq#0#0@@Z#OOGOOOj;;lNHtCG68ee969GOGO9OG969GOGOGOG68Zv.;vvXAlT;yCqGH,;AOGbXrvlyAAkkmkkN4X4N44mHNlAcjmtjbpbtjctcjjkLhmp@OqZ#ZZO#DDOOGOy ltOGcv,;yHb2mLyVyVyyyyrVlvlvv;v;;;;;;;;::,::v;v;vvvvyvvvvvvlUcptptjmmkHHccccjqZq@CZqO#D#OZODODODO#D@ZqZZ6D9Dtmy                                                                           
lvvvvvv;;;;;;;.........,.,:;:;;;;;;;;;;;;;;;;VvvvVhbC0C0CqCqCqC@C#0@@#@D#DZZ0#qZqZq#0ZqZqZ#969qqtrlhDe96G6O69G96O9O9O9OG9G9GO9OG9BBZ;.:;;vmjCcA2G8cvvMj9jMlv;;;lktmmUUhUUk4HHkwrMtHHmjjjttcpcjbqpkNkkpZZ#Z#D#DZDZOOecvycmD9CV. vmjXrMAyryLYyyVvlllvv;;;;::,;,:,,,;;vvv;v;;;;;v;vvLtptbpCtjcjttmtctj#qD#ZqZ#OZODODOZODOZD@Z@q0OO9GOktU                                                                           
Vvv;;vv;;;;;;,. . . . . ..::::;:;:;;;;;;;;;:vvv;::vUbbqC0bqC@Cqb@C#0Z0Z#D#Z0#0#@Z#Z#D@DZOZOOGDOZOkXUDee99OOOGO999OGO9OOD9O9O9O9DOe80,.U0mtCXr4NX#X  vrTtmyvvllvVk4hAX4kUHmcctmmkcmmkcc2tjccbZqOObkkUkHOZZ#OZDZOZODOOCmqAvMODqr  vA4AMMMrArALAyYvVvVv;;;::,,,:,:,,,;;;;;., ..,,:;vrjj2jtcUyAhNjbjpmtp#qZqZ0DDOZOO9DD#OZD##qZ0ZZZjjpmC#,                                                                          
lvv;;;;;;;:;;,   . . . . ..;,;:;:;:;;;;;;;;;;v;;::;Ub#C0C0bqCqC@00C@0@qZqO#DZOOOO9O9O9DOZODOZZ@DZ@0@OG9ODOZGOOZOO9OOO9O99GOGO9OOZGBG.;H#COpcHkUje0;.;vvwrVvrrMAwVv;vvNkmmcccHmHcmckHmttjctHqODZOtkUkXmDOZDZDDODOOOO9Zpmy;vLm@DA;;;vwXUhhMww4hTYVYyYv;;;;:,,,,,,,.,,,,,.... ,,,,vvyktmUArvv;:.rp22pjCCZ0Z#DZOZOZODDqZ#Zq#qZq#ZgZtmZbCpv                                                                          
l;v;;;v;;,;;;,. ......... .,;;;:;:;:;:;;;;;;;;v;;:vw0#qCqb00@C@CqCqCqb#CD#DZOD9O9O69G9ODOZOZDqqqOOG9O9GDOZOZOO9O9O9OOO9O9OGOGO9Z#C82;LH4jCLlMk4Vl;;;v;;vlvTk4Awy;.,;YwHmtctmmkmkmkHkttjmCbpCD@ZZ0kHN4UOZOZO#ODOO9DOO#;,MVvVy4tbAv. .vhkhXAhh4rVYyVVvv;;;;,:,:,:,,.,.,.,....,:,;;vvVrhylvv;; .:NjCZO#ZCZ00C0b@q#q#q#qZ@Zq#0#0Z9Gbb#@p9p                                                                          
vv;;;;;;::,:,:.  . ........,:;;;:;;;;;;;;;;;;;;;;::ytD0#b0CqC0C@CqCqCqCq@ZqZ#ZZOOOOOOODODODOZZ@DZOZOZOO9O9O9DOZ9O9O9O9O9O9O9OOOOq9Bk                   vywAhyyVv:,;vykkcctctmmHcHmmtjjttbDZO#ZZO#CtHw4@O#OZOZD#OZZZGpVw2vvyLyrh4yv..,yTXAMAMrAVyVylv;;;;:;,:,,,,.,.,.,.,.,,,,;;;;VYVYv. :,. :;vkUHUXmCt2cjmctCb@C@qD#Z@Zq#0#q6g9200qO8t                                                                         
l;v;;;;;;::,:,.     . . . .,::;:;;;;;;;;;;;;;;;;;..Y#D#0qCq0qCqC00@C@0qbqC0Cq0ZC#C#0ZqZqZZD#DZOZOZDZOZOOGO9OOZOOG9GO9OGOGO9O9OGOOB8M                   .rAVvvvv;;;AhUkcmcmtHHHcHcHtpCb2jZ#O#DZOOO0bHUXDOD#D@Z@D#D@OZOO8OAYrLryLAXAyry,vyryTrryyVyVyll;;;v;;::,:,,.,.,.,.,.;,::;;;vvv;;. ;,:vkyvvvvVvv4kcjtjt2pCCC0#qZqZ@#q#@#@9#0j0bqg8                                                                         
vvvv;;;;:;:,,,.            ,,;;;;;;;;;;;:;:;;;;;;. vtOqqC@CqCqC0C00@Cq0@bq0@2qC0CqC020Cq0Z0ZZO#OZDZOZOZODOO9ZDZG9G9699O9OGOGD9OGZeB4               .    ;VyyvvvvvVwXNkkHHtctccHccccCb0bqqOZD#DZOqCmHhUqOZZ@Z0ZqZ#OZDZGG9HVvATAyrrTAmw:.vvXkhyVlVlyYVvv;vvv:,,:::,,,,.,.,,,:;;vvvvvvvvv..VvvUHNyv;;;:.;vUNUhcCq0ZC0b#q#qZC#@ZqZq#bctCbBB:                                                                        
v;v;;;;;;:;,,..     .       ,;;;;;;;;;;:;:;:;;;:;...NDZC@Cq0@0@CqCqCqC0CCb0CCpCb00qb0C0C0C@0ZZOZOZODOOGO9OODOZOO99GOGO9OGO9OOOGOZO8h                   .;vlvvvvrXUttpctkmmcmtctmtctjZq@CZZO@Z@Dq@b2H44@OZ@Z##0ZqZqZqOOGgGyvrUrMryyrrMv. ,lAVYvYVyVylYvvvvv;,;:;,,.,,,.,,,,;,;vlvvvvv;: .rYyyyYlv;.  ..;vvv;TO9CCZ##@Z#Z##qZq#q#qZ@Z#9B@                                                                         
v;;;;;;;;;:,..       .     ..;;v;;:;;;;;,;,:,;,;:, ,46Z@C@C@C#0@00C@C@C0C0bqC0b0bCC@Cq0@C@0@0#0ZZODOZ9ZOZGOOZOOGO9O9O9O9O9OG9GO90Oey                :;,vvvVvvvY4ktm#9gObmHHtctcttptbq#0@0Z@D#ODDDOD9ZZCDZZqZ0#q#0Z0Z#9Og9L;hUAATvyVyVrv. .;vvllyVyVyVVvlvl;::;;;,:,:,:,:,:,:;lllvv;; ..;VA;;;vv:   .  .;:;,vwH;XDZOO@Z0@0#0ZqZ0#@DG8BO                                                                          
v;v;;;;;;::...            ..,:;;;::,,,,,,.,.,,,,:,. v0O0qC@Cq0q0@CqCqCqb0bqbqbqb0C@CqCqC@00C@CqC#0ZqOOpVUO6DODG9GOGO9OGO9O9O9OG9Dq8Z                ::   ,v;;VDOCHmckTUcjkmmttccjtCCq0#qZqDZDDOZODODOZZZZ0qC@C@q#qZqOO999V.vUrhAlVAVyyryl. ;vvlvvvlYyvvvlv;,;:;;;:;,:,:::,:,;;vlV;.   ;vyVv;:,,..,;;: .,,.:;;,.LpUOZD0Cp0bqq#0qq0CBBm                                                                           
v;;;;;;;;;:,.            ..:,;:,,:.,...............  UOZ0@C@000@0#C0C0b0C0CqC0C0C0C0Cqp0bq2Cb0b@CqC@#CrVytOGDOO9O9O9O9OGO999O9O9Z#BD                      .;;lZBBee9krvyhtmtcpctt#q#0ZqZqDZODDZDZODO#D#D@qq#qZ0##ZqZZOO6Gqv,vArrvyryVryAMr;::;;vvvvvvvvvvv;;:;;;;;;;,;,;:;:;;:      .vvv;;;:,,.,.:;vv;,,.,,;, .wvvj2j0qZ0qqZ@Z0#ZBD;                                                                            
v:;;;:;;;,:.             .,,:;::::,.......,...... .  rG@qCqCqC0C0b0C0CqbqCqbqb0C0C@Cq0q2CbqC02qC#bqCZkvvvYpgGDOO9OGO9O9O9O9O9O9OGG8H                          v,vpGGOjDcrvU22jjjppCC#q#q##OZOOO#OZO#ZZD#Z#Z@ZqZ#ZqZ#Z#OOG9t.,wTAArArArArwAy;;:,,vvvvvvvvvvv;;;;;;;;::,:,;;;v;,    ,;vvv;;;;,,,:,,,:;;;;:,.,:;;yv :XvljBGD@Z#ZCqOBm                                                                              
;;;;;;;;;;,.             .,::;:::;.,.........,...    vZOC0bqC0bCCqb0C0CqC0C0bqC0b00q0@CCCCbCCqC#bCbqjAvlvvr969D9O9O9O9O9O9O9D9OOZeBh                             U6BCmZ9HlVkkmH2tpjb0qbq0OOOZOZODO#DZODO#ZZD#OZZqDqZ@OOO@Cpy,LrArMrALALAryyylv,.:vvvvlvvvvv;;;;;;;::,:,;;;;;.  .:;;v;;;;;;;:,:,:,,,:,:,;..,:lNT:.:l; UO@ZOqZ0qg8X                                                                               
;:;;;;;;;,:.             .,,;;;,:,,.. . . . ... . .  ;ZO0C0CqCqbqbqCqbqCqC0CqC0CqC0CqCCb02Cb0CCb0b0bbLvvr;vkG9OOGOGOGO9OGOOO9OOZOe8v                             U8BbUC6@MAwXAAHCtj2D#Z#ODGO9OGO9ZOD9OGODqO#Z@ZqZ@Z@ZZOO9qjkwvvrTrArArMAMyMmmy;,  ;vvvvvvvv;;;;;;;;;;:;;;;:    ,;;;;v;v;;;:,:,:,:,:,;,:;,,v;ykk:,:Nv lUACbjZBB8y                                                                                
;:;;,;:;,.         . .   .:;;;:,,,..     . . .   .   ,jOC@CqC0CqCqCqCqCqC0bqC0C0CqCqC0C0C0C02C2CbqpqbMvYVv;Vm9O9O9O9OGOGO9O9ZOZOZ8B;                             tBe0pbe9mrNwrVhktmt#B6GOG9G96969G9G9GGO@@ZZ#Z0#q#0#qDZOG89HUAvMrMrArMrMrrLArylv;,,;;vvvvv;v;v;v;v;v;vvv;.   ,,:;;;;;;;;;;::,;::,;::,:,;,.  ;wr;.:VV.lrVr2eBH;                                                                                  
;,;::,:,,           .    .::;:;,,,                   ;C##CqC0bqbqCqb0bqCqbqb0b0Cqbqb0CCbqb0bCpCbCC000VvYyvvvXOeO9OG99OGOGO9ZODO#O88                            ;V0eqCjqOGbAyrlllrAMACBBeBOG9GOGO69G9GOGOO#D@#q@C@@Z@#ZOD6eBUlMlLTrrrMAArMrLvlVryv;;..;vvvvv;;;v;v;;vv;;;.   ,;;;;;;;;;;;;;;::::,:,;::,;::,  vrv ;;YrvvhAme8H                                                                                    
;:;;,,::.          ..... ,:;:;....       .          :cO#CCbCC0b0C0b0C0CqCqCqC0Cqbqb0C#202Cb0b0Cq20CDjvvVvvvvvDB69GOG99O9O9OOZOZOZ8B                           ,LXDOqjC0#@#tkXrv;;;,.,vA#eBGGD9OGO9O9OGOGO9DDqD@Z#ZqO@OOO9ee#rA;rAArMrTrMrMryVyLAyv:  ,;;;;;;;v;;;vvv;,     ,,;;;;;;;;;;;;;;;::,,,:,:,:::;vvrVv  ;vvTUZcVly                                                                                      
;,;:;,:,,.,.      ..,.,.,:;;;:. ..          . .     H9C2q2qCC20CC2qC0CqC0bqCqCqC0C0b02CbCb0CCCCb0b0@jvvYVvvvYc6GGO9OGOGOGDOZODOZOB6                             Abqb0CqC0#Ojmr;         vj669DOO9O9#9OGO9OGDOZOqZ#DDDZOZ9OOOGpryUMYyhAMAALArryLyryy;;. ,v;vvv;;;v;v.     .:::,;;;;;;;;;;;;;;;:::::::::::,,;;y;  .:;y@BL                                                                                         
;,;;;;:,,::, ... ..,,,.,,;:;;: ...       .   .     vqBDOZCjCb0b0bCb0C0C0C0b0b0C0bqCqCqCqC02CC@b0C0b#tvvVv;;;;rcOG6O99GO9OOZOZOZOZ8b                            ;C#mpCC2q0#0p4r;           y@GeOOZOZOO9O9OGOOZODOZDqZq#@OO9OOGem vHrlvLyyyryrLrVryArTkA;vvvvv;vVylvv. .,:::,,,,,:;;;;;;;;;;;;;;:;;;:;:;,:,,.;T,  :v:vbh                                                                                          
;,;;;;;.,,:.. ..,.,::,:.,;;:;,....      .   . .    TgggG9bCpCb0bC2@b0bqC0C0Cqb0C0b0CC2q20b0bCC00qC@@cvvlv::.:vX26GGO9OGO9OOZOZDD9BO                            HZtcbZ0q0ZC2jmV;            ;cZ6ODZOO9O9OGO6O99ODZq#qZ0#O9O9Ge9U  yOklvLlryryryyVryMAhmr   .:;yTYvv;,;;;;;::,:,:,;:;;;;;;;;;;;:;;;::,:,;,:.  v   v;v;v                                                                                           
;,;;;;,,,,.,...,.,.:::,:,;,;:: .         . . .    v#OGGeO02Cb02Cb0b@b@b0CqCqCqbqC0bCCCbCbCb0bC20b0b0hvvYv;;;:vlk9eO9O9O9O9ZOZOZOO8m                           vCZCjZOZ0Zq0tpXv,              vcOZOOG9GOg9G9GO9ZZq#q#q##OO9O6DOc: :rtHwXXLArrrrrXAyL: r:   ,rV,   ,.;;;;;;;,,,,,,,;;;;;;;;;;;;;;;:;:;:;,:.  ;,   v;;V;                                                                                           
;.;::::.,.,...,,,,:,;,;,:,,,:..         . ... .  ,9BOZOOOC020CCbqCqCCbqCqC0CqbqC0Cqb0CCCqb0C0C0C0bqCNvlvv;vvvvvTe6G99O9OODOZOZOOBZ                            .AHcC9OmccjctjXY;                megGGe9g969GOO00CqqZ@Z@DDODO9OOqAL,.v44wAAyArrAXkUyL,.vv. ;jOt,;,:Vlvv;v;vv;;:,:,:,:;;;;:;;;:;;;;;:;;;;;:;;;V;  ;v;;yv                                                                                           
;...,,..     ..,...,.........                    tB9ZZ#ZqCb0CCpbpC2CpCp020C0C020202b2CpC2Cbbpbp0b00CY;;v;;;v;v;lkDO9ODDOZOZDZDZOgk                              ::;rkAAAyTNAvAy.               vGB9999OGOG9GZ#bCb#@ZqZZOZ9Z@0OpMr; .;vvllylVYyvvlLMky:,wwwNhv.yXXmvvvvv;;;;;,,.. ..,.,,,.,.,.,.,,,.,,,.,:vvl   ;;:vH                                                                                            
*/

// A still taken from Aubree Dieckmeyer - Feeling Good - American Idol 2012 - San Diego Auditions - 2012.mp4_ASCII-aerosmith3_300.txt:
/*
                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                            
                                                                                                         .  ,vl:    VrVvvv;;VYyyV;,.,.. . :;,.,                                                                                                                                                             
                                                                                                    .vvkp0p2tpcXv. vq6tkvvv;,,.,.,.,;vVyrwNcjCtH4k4khhLyyl;;;;,:..                                                                                                                                          
                                                                                              :;YUttbZZptmttC@Op4;  yTAAVvv;;;;;:.,:y#O@qjjtjjjXhT4NHmcctmtcjtccpcjmmmcmcttNUVv.                                                                                                                            
                                                                                         ;;Mmtj@jjp@Z9GpTUHtjccy,,:;vvv;,v:. .;;;  ;C9O9ZOq0C0pH4kUmHtt2j2bCpbjtHHUHUHUUXXw4XUUcANAv;;vv                                                                                                                    
                                                                                     :v;VwNrrATATwTXjjHtjkUHU;   ;Vyv;v:vv  ::;,  .jBODq#@#CCjpttctkkMwMwUkUHUmmcctmtmcHUNhrTAMrTMwwUArvv                                                                                                                   
                                                                               ;vNUUH0OGZ0UAVrrAAMvvlAC90mYv.   LvvvvvVwv  vHY.,  v8gDq#qqpC2q0Z@Z0CjjmcHHUHUkXmkmHHkHUHUUwAyMrryyyAAAlv;v;                                                                                                                 
                                                                         ;v;lATwUHUyv;;vH4NyyyAMAr4;  me90;    vAvvvvvT;  ;tA,;;  :Be#Z##q0b0CZ@ZZOD9O9OOZZZD#Ctjj2tjk4wUUUNXNNyryArhwAVyLrv.                                                                                                               
                                                                     vlAYMMUkkrLyAwmv     vXMwrTyymH  ,9Bk   .kMvVlYvvN,  vUY:vr   8eO#D#D@#0Zq#0#q#CqCqb0Cqb0q#0qbC2jmmUU4kkcUTMkUwVVVVYyyv;:                                                                                                              
                                                                .;;vAYlvywUUtUm4kUMH2,     lHXwryLUH   C8L   Yeyvvlvlvwv:vyhHVVV,  v8gOZ#q#0Zq#@#q#0@bqCCbCjbC00@0@Cq0#q@0jNUkHyrANArlVllvv;;;;                                                                                                             
                                                             ;vyvvvvvAUpqZqq0q0#qO4kbv      vwhMyrAp   jB;   y8rvlvlvvLAVyrUhyvVY;  D8gGZZ@ZqZ@Z#ZqZq#q#0@C#000qCqCqC@2q0ZbjmcUvvvYylVYlvlvv;;;;                                                                                                            
                                                           :;.,.,;VH@b@ZqjbjpbqCD0Hmt;      .rAyryXU  :qGh   Y8rlYlvlvymN;,vrYvvyYv  V0Zeeg#ZqZZZ#OZO#D@Z#ZZZ0@bCj2j2ctcpj002ccV   ;vvvlXMAv;;;;l;                                                                                                          
                                                      .;v;   :vN2cktctmctjctcjpC#2vTm,      lTyryyUA  vO0Gq  :BAvVvlvvlHA,  ,VlVvvyv  .v28BGDOZOZZZO#D#OqZ@#q#@@qZZbtckcctkUMMV       ;ylyYyvv;;,;;.                                                                                                        
                                                   ,vv,   ,vHq#kUXU4U4hrj0C2C#Oq0tyVh,     vUyyVLyUy  cDmbB;  6DrYlvYvllYVv .vlvYv:;v;, ,VZeBGeZD@Zq#@Z@ZqZ@#@##Z#Dpbjjt2cUhTvv       ;;;,..;;vvv;v;                                                                                                        
                                                 LAv    v20qcmUUUU4kkcmHqO@Z099Dptwylv;;   yhAlyVkY. vDqcHUt  r8cMvVllllvyyy;..;vv:;,vv;   vXDeeOOZZZD#Z##qZqZ#DqDq@0ZZD#OZObHM.      ;rv;;;;v;vvv;;;.                                                                                                      
                                              .vv   ,A0jtUU4HXXwwNkpZC@C#0#q#DGZqcvvvvvl. ,VhryVyLv.vXcmbUhmv v#OTvYVYyvVvVyV;:,:;yv;    :; ,vVGBeBg6O9DDZZq#0#qZqZ@Z##qZ#DZb4T;      mOO0myyVyyy;;;vv.                                                                                                     
                                           ,lT;   vXtjk4TMAUhhAXUkHDDD#Z0@0##OD#tUYYlrLY  vALrVyyvvvykUmmkTkXYVcHUrLvvvAVVvYvvv;,.,vYV,    .:;,..LTcBB99#OZZ0#@Zq#@O@D0Z@#q@thv;vy;  vjeGGOZCqCqCmv  :V:                                                                                                    
                                          vV;   vjqmHTyvYvyAhTUUctqDO#ZZZ@@ZOCtMYvALyYv. ;MyYYyvVyUUHhUUkUkkcHmUNkkwrv;;vYMVYvv;,    :v::       .,  :yAN@B8ee9GO9O9DOZZ@Z0Z0Cjk42CjHkUtp2j#ZODOOeGcv  ;Vv                                                                                                   
                                        .Y;.,vyUctMryryryryMMkcjpDOOZZ@ZqDZOjUV;;wVv;;  ;4yYvyvVAHHcHk4kUkUHUkX4wjjmXA:..;vVylvv;.      :vl;.      .:vvvvkhkcODO990999O9#0#9OO0qCqCZqqtccbbqCZZZ2cUjTv:;Ly:                                                                                                 
                                        M;,vXUcmHAlvYvvLhMhA4t#Z9OOZZqZD9OO0cyY;vry;:,. ;LylVlvlHHkXUXHkmmHkHkmUHtbctHkVv.  ;vllv:;,       ;;;:            ,:vMyMVvj9ZOOeeGZZ0Cj2tjtpjjmbZZq#@O@qjCO9bkrUUwv                                                                                                
                                        wrwkwAyVvvvllyLwAUHctDD9O#pqZ66B9ZcwlvvvvV;  .;vvVvlvlvYNHUUNkkcmtmcHcHmHjtpjb2cLv,   ;hyv;;;          ,;;:,..        ..;;;v;vvvvvrpC6BeGDCpjptp0D@Zq#ZODGG9OOOZCmUH,                                                                                               
                                       :HwNMAyyvvvYLXHckc2O#Z#Z@@jjpO9OCcrv;v;;;l;.  ;VYlvvvvvXmmUHUkkctjjCbCjpmctbj2tjtcwAv   ;lvvvv.           .,,;;;;;,.                   ;CgB8gOC@0ZZZZZ@Z#OZOZO@Cbqpbtv                                                                                               
                                       VkMyArrYlvlAHcptp0O9Oq##ZqDZOOCMLy;.::;;;;. .;yvvvv;vvvXckkHmHmktckArlrkbqOODqDCCttctcv   ,vMXv                 ,,:;;;v;;.                lHGeeOOZOZD0ZZOZO@O#Z@#bC0ZY                                                                                               
                                      YU4rYvlvvvyMccHcp@OZOODZOZZ96OjVvv. .v;      .;vvYv,..ykHwUHmcp4yvv;:,,.:,:,;vr42qCctmbkv.    ;VMlv,                . ,:;vrrl;v;v;vv;.        yC8B8ee9GOOZOZDZOZ#qZp0Bm                                                                                               
                                     ;TAXAvv;vvANcckkj0Z#O#OZOZ#C@ZGtU;   :.      .:;Yvv:.,VkcHmmkNNVv;:;vvAHchl,     :VT2qZ2t4wv.    ;vVwy;,;;;,.             ,;;;;;;;;;;;;;;,,:;.    VmZ68BeO9DODZ#DZOOGB6v                                                                                               
                                     :VyhVYvvVNUct2kmmDZZ#O#Zq@ZOe8Dv             ;vrTv  ;Mc@ctmkrl;, ,M0jCpttjjCckLv;   ;TcZ#O#twr;     .;vylv;;:,.           ..;vv;;;;;;;;vvvvv;yAy.     yj9888B869OGg8B0                     :                                                                           
                                    .;yUvvvyMkXcpZZZCZZZqDZODOOBB8p    .        ;;;vlv  :U0Z0mry;. .:vAq@qCCpjcpj0qOD@M;   .;LMbCbmHwl       ,:vv;,..,,;:          .:;;v;v;;;v;v;;vvlVYv:     .vHZCeB8BgDN                      H                                                                           
                                    ,vAYv;vVAkttDZOOODOZD@O#09BO#v             .;vy;.  v2ZqHwY;. .vyk2bt2cpttHcHtcpj2j@bkV;.    LmjpqbtXv.      :;vvvvvvv;;:;,,       ;;;;;vv;v;;;v;vvvvVVV.                           HjmtpZk  hv                                                                          
                                    .vlv;vvyycCDOOZDZOZDZOOOj60,             ,:;;;;   YjptUL;;. .kjpt2tb2jt2cmHcHcmtcmkCOOmjv    ,;l4tbqjckMv.    .;vlvvvv;;;;vv;;.        ,vv;v;vvv;vvvvvvyVYvv                      ;GDtt0CCr ;p                                                                          
                                   ;vv; ;lwm##ODOZO#DZDZ968gCv    ,         .;;vy.   A00jry;,;vVtZCtbjpj#00b#0CbCtptjccmtjp4hwUT;.    ;vhtqCCUA;        ;lYvv;vvvvvvv;        ,;vllvvvvvvvvvrrhhrlwh;.                HBCUmkHHkrket                                                                         
                                       ;rU2OOODOZOZDZOOBB8Bv    :;          ;;..    kDpHMl;:;VAm0qppjpp0C#CDO99Oq0tpjpmcmHkcHppCjtr;.    ;vXtbC0jXv:      :vvvvvvvvvvv;;;;;v:. ..,,;vvlVlYvlvvvvvlUOC@0CqCctmc,      ,BgkcHcmttCj6g                                                                         
                                       vtm#6GDOZDZOOg6B9X     vv.          :r;    l20Hrv;,;vAHtb0jptpjC0ZCZ22pCUwNkUmUHkHkcqDDZCpccHcky;;...,;rHCj200hv.      .;vvvvv;vvvvvyrvv:;:::;vvvvymmtkUTmjpCjHtcjZZUUw       k8tHHcmtcjmccDj;                                                                       
                                       UZCt6OOZZZD988#v     vVv,         .:lT    X0CAv,.  ;AU20qtjtbpqmmkHmNVrrwMNwhhUUk4kXhwUUjmtj22CtcUNAy::;;;VMt2G9tyv        :;vyrrvvvYYVlyrAyAyLYyVYvlvvlN4kkHpZb2HHY;,,      :eGHHtctctmcmc#8Gl                                                                      
                                      Aq0b0O9DOZGe89;    ,vYv;.         ,:,     pbkrv.  .lj@CtCtTkj#6@4AXw4wMANw4UUUHkkUkX44HXh4cHcctcjcjHckAyAyv .  y096ZbX4vv       .;vvVlVvyrwATATAAAAVv;;vvYyyryv;;vll;..       NB9mcmtctmctjk4UtA.                                                                     
                                     vC@tCCOD99eBO;    .ly;v:.         ;v;    .pp4Yv;;;lTm2C2CjCtjmtjHM4UHUUhXwUUcXrVThkHHkmHHNwcb2bj2pCjbjtmmHmAl;;   ,vhZGD9O#mwv,     .,lvlVrrrrArArAAMrMAhrrrMrwv;vyrl..        98tjctccckmCHylllv:;V4y:                                                                
                                     bDtZ##0O96t;   ;yUVYv;.          ;V;    UeChl;,,;hqeOCcpjpbO2mhTTHkkUkN4XXNtp4;vTHHmkmXUUkkccptjctjjjpccHcHHUHjZl.   ,LUp6O900ttkTv;..;vvvvVYyyrrMrMATATrUHUhhMALV;;          ,8@TkcmcccXmjHVyrr;,,VUtr.                                                               
                                   .p#kmDO09G2;   :Yvv;v;:.         ,Vv    v2DCw,..,vUjCCCppcpjtccHHUmHcmHUkUmXMp9wVAcmmHcmHkkUUhUXUNktppC2bjbpjHtctmchAv,    .lXp2009DpHkMrvvvvvllVyArArMrAAMM4UUUw:..            vebNhHjcjmHHbHMYryv. .;vv.                                                               
                                  ,hZHUC9C#Bp   .vv;;;;:..        ,,;     UDGy    .h99Cp2jjtpjbcmHcHHUmUkUUXHNNAqemAUccmcmmkHUkNNrryVkCj2tDGeggDOZZb9eDbHXhl:    .vVwkmj@qOtHUHHHX4ArVryAyryAMArrvv;,               rmDtmcjcjtpctkrvvvvvyv;;;;vv;.                                                          
                               ;vLUjbCbDOCjcY,vyV;..;v;.         ,;,    vmqby     vGBGjpjbjbj2jjHmHckH4hrwNHHmkHcbUXctmHUUXUrrrryylVlX4U4UMXkcc#O9DZD9CZ@#CpjbH;,;:,,;;VLhNcpjmpttHmHmUHUUhUNUTAYv,..               ,yOOmttjjbj2ckTVvLrLLv;;;vvv;:,.                                                        
                              vUv:UckcOO9bklvvyyv;;,,..       .:lv    ,h#CH,     :He9jjtpj2tjtpHmHcHcHtkHkm2OOOq2H4UcHmHkkHkUNXwhMNTwMXNkhrlAhccttttjtbbqCpcjt0mTLyvvvvvvvvY4hkkcttt2jpjpjjcj2OZ##CHHHcTlvv,;v;.;vvlhX0OOCbcpjbttUUwwwUMlvv,,;v:;:;;;                                                       
                              OO. Lck200tHyvvvvvvvv;..       .,;v.   yNGO;      vmpqCCjpj2pbckAXmtmcHttCCbpjmmkj0ODGZbkk4UUctpjpctHmHUhUN4ALAHcHNMAXUbbCjtmcHcmtcjHU4UrAAMLAM4UmctcjcjttmcHcmkv.  ,h4mG888BeG8D99G9e69bO9OC#0CttcHwhTXh4hHwv ,;;;;;vv;                                                      
                             ;8t  #Hct@kYvVvlvvvvv;,.      .:;;.   ;hbCjv   :.vyj#0jq0Cjbjbjj4AhjctctctmjctHHUUXjb0CqCZjm4UHj2CtcHmmcU44HkUThXkNhrAhcc2tjcjjbpbbqqZbCttcmHmkHUcmjtjtjctccmcHccjwv.               vt09G90@Cq@D@jUttchhwhAUq0Uy:;;;;;;vv                                                      
                             kB. Ygjjp2hvYVVvvvvv;,.    .v;.;v   ;U#ZtX:  ,::htb2Zb@00jp22ttHkTUcjmjctmcmmHmkcHHkcmtHjCCjtmttCb0ZOOO@CmmmcmcHmHmHmHmHcmtcp0D##0#0ZqZCqCq2CjpjjccmtcjctctcjcpjbCZDGOtV;          .VVlVwc9ZOZOD#cmHtcUh4wwX@GOjy:;;;;;vv                                                      
                             B8  lqkjbtlVyylvvvvv     :;vvv;,  .vkpmA;;..;VyUUmCC2@0#bCjCbb4UXUXccjctmtmcmtHHHHkjjtm2mUm2@OCjj2jbjpjq2cHHkcb@qDZOODCpmcctjDZD##0Z0qCC22ttctHcmtHcHtccctcttjccHcmtcC2CCDZOODHtbHwY,   :UO9CqCZZZpjHHXUXUXUUmcpXVv: ;vvv,                                                     
                            ;8B  ycttjXVlyVYvvlv:  :;vvYVY;. .vkjbcL,;;;Vyrm0CbqCq0CjCptcbcXwU4Uktccmjjbb0ptmcmcpCTAc#tXMpCUrwNXMN4ct0jpjjtZ0bcmctmckmkHHccjcCCODOZZq0pCjjctcjctHmcttptjtptjctcptptC0#q#bCp#@#bmv.:vlNC9jkkHH0DCmHUkUkXUXUNUHpHy,.;vvvvv                                                    
                            TBD; ytcmMlvVYyv;vy; ;yYVYvvyv. ;yctmNwyLrAvANUcO#Z0##ZjmkmUkkHhUXUUkHcHtmUw4X4ktctcjmcmct0TrcqMhwXX@bt4UkCCqpCbj4MMXh4wUUkUUXUhUUtqC2#ZOZODOZZCq2Ctjj2j2tjjjtjcjtpj2jpp0b00CbCpC2UYrUjqZC0bjHmmtb9#qtHhXNkUHkU4mmjhv:;;;;v;                                                    
                            v8OkTkHjHMvvlYlVvv;,.;Vyllv;:,:lMUkchAyAAMrAwHmbq#@Z#9#pUUUkHmkkXUhwwHHHHHUkUk4mctcpUUqZUUpUAj2UNkXXctHkMwhkUHk2jkA44kXUNUXkUk4kNXwXAwTkHjjCj0#OZZZqpC22jjtjttmjtjcptpjb2C2j4UXkHjkAN2qD0CtjccctctpZD90jtckUUUXUXHmmy;,;;;;;                                                    
                            .66q#jHcHAVvllYly;. ;vVllv;..:Mcjkk4hyrrAMkHjj0@O@#2qqqtckcHmkH4UhUTUUHkmmjjbp2pbtmNrLHtpmcNUUmNU4UNMTHt@CUAhN4c2mkUmHmkcmHXU4kXUhhrArArXXU4mtpjpCDq0bCjptjttmcHtcjpq0qpCbbm4TXhHcpjCDg#Cjjctctcm4UtGee@pcHhUXUwXUtcA:.,;;;;;                                                   
                              yjj2prlyYVvYllvvvvvvvYvv:;vkbCmk4kXUXU4HpZbCpZZObtmHXkkkkHUHkhTkmbjcUHkmHtccmCbpkkHjb66bUHkkUHkHUUhkmb0pXmkcmmmpjjmcmtjbmkNUXUUU4k4UXU4U4kNNMhh4cq0qCZ#Z#DOO@Z#ZqZ@#0bjCpcMAAw4ctCbDqjHtctmcmtHUhjOBBZtmAAMXNUwNkcy;:;;::;:.                                                  
                              ;0ZkUvvvllylYvvVhTrvvvv;rXmmtHHNUUHHmUtcqbbp0ZOqbct4kUHkkXkkUXhNm0#cmkmHcctcjhNmjbpXHtDjH4mmccjt2cwNcj#jUUHUmmjcjcjccmjjcUkNkUkXU4HUU4UUUUUNUh4AMUcHmt#Z9O66BBeG9DZ0qbbpCj2jjcjtCq@CjHtctctmcmm4TUqG8OD@@pmMUUkN4hUUMVv..:;,                                                  
                         l,   9ecyvrrAAAvvvlVThXyvvv;rb@cmUUXUhkUHkHcjj022pOOOtcHHUmkHmjmmtbjpj#0pcccttjtjcH4cpqtkNUmpmUwUcZ@#@0w4HjtjkmkmHHmttmkHHcctHHUkUkUHUkXUXU4k4UNUhNwhMhAhAhAhMkHjjbb@qOq#CCbCbZ9e6e99q#@OtjctctHmHcHHT4t6e6GeeBbUUkUUwhhkyv;, ,;;                                                  
                        keD,:98OjkmXkUcy ,yUpMryyvVyUmjcmkmX4XXXHkHUccjC0pqZ9@jmcHHkHUj90k0Cq2ptjjptptpc2tppbtjtjcjmj2jwAw2tptjtmUmkHUmmmUkNUj@AvrjqpUHkHkHUkUk44wUXUUUXUX4NXTkHHUkU4TNHmhXwUktpC2Cbqb0CZ@Z@O@2jCj2tjttmtHcHcHUUpOeG6969CwUHHNUhHXLVv. .vv                                                  
                   vq; hB8GOBqrmOGjX44XM;lMmHyVyyArmjpHHkckHUkUmcjcccptCC0b0jtkmmHkmHUAp4yUkNwATXpjptjtjt2tjccmcmccctpchymjkXU4kkmkHUHHmHckNrkCqtcTrwHkHUkUk4k4kXkUHUkkHUUXk4UUHmmkkhNNjpHwhXUUHUkHmmcmmkmkHHcNUkmcjpCcjttmtmctpjCqODGOOD#tjUNXkUUwwV: ,;v                                                  
                   p8cNOejjg8y  @VYXmHkc0cUyyyrrrNcpCtjHcHHkcmcc22btbpC2qCCtmHcmcUHUHUMUhywhkUUUkcptttjcjttctmcmmHcHmmcmhH0UkUkUHHHkHkmHc0C4LycC0cUrXkmkHUHUmmcHmHmHcmtmHUkXUXkHmHmUUNUcbmUUkXNAryMrwAMAwANMhUcHmmj2Cjjtpctttt@q2mp#OZO9g69qprN4HUkH4v  vv,                                                 
                  .h2BBpjbjZB; lD:;kcc4HmH4kUcmmHctptjmmkcUHHcHccbjttbp0CZCCjpcckkUkUHkHkkUHtjctccctmtctccmcHmHckcHcHkmDHU2mkHHHUHUkUcctmckccbcHmttptmHmkHkkkHUH4UNUXk4UXmU44cttccHcmmUtjtmttpcXrhUhAwTXhUNXhUHkUHc00qj2jttbtqZDqC29ZODgB8e6#mThhUk2tA. ;y.                                                 
                  kl2egcpCCb9Y;CqvmtHUHkHkkUHkmkmmtmcHcmcHmmtmctpbCjCCZqCcmUHkH4UUkkHUHkkUkktctmtccmcmcmtctmtHcmmHHUHUtZch0HmHmHHkHUmctctThH2CjmcctccHmmcUUcjHUHmkHmNrNUjb2hUc0jjccmcHHmtcpjCqCNTkbXhMhwk4UXUwNhUXmtbbCjHhNMXMyvycCCOOZC9OGO9#bkUMANcmY.;v.                                                 
                ;GO;Ng@p2CqCcctCcccckkkHHHUHHmHcHcctccmHkmHtccm2j@CZ#DZZjHNXNNwXUHUHkkkHUHUmmjccmcmcmtmcmcmmmmmmUHHmUUmDUN2tUHkmHmkUUjcjcckmHcHcctmcHmHcHHNjbt4bb@qZkXcCqOCkwjpjccmckUUUHttjpCmwrkjtwwM4Xk4kXUhkUHNAyyyrlVvYY;..;Hbq9e0mj0qOOOO@ckrAr4Uv;v                                                  
               lOBO,T9CbpC@tbmkUkmmHHHcmcmtccmtctmcmcHmHmHtcjcccq0Cj#Z9#CcHNUXUwUUkUkUkkH4UHpHUctmcmcHcHcHcmcHtmH4mmjppmXX#bkMHjZ#jwkttcjp22pctctctmHkcmjttjCtcjq9OpkcZCjcbtjtjkkUkmbjc4mtjjptmhh4cmmkUw4k4XHUwAwAMlv;vvvvvvv:..,vk06eOcqq#ZOgBDZ@XYrrUvvl:                                                 
              ;8GZq;yg@qj0pbpc4HHcHcccccmtctctttccmmHmHtcptC20pDDZpjkjmkUUNUNUXU4kUkUUUHUkUjbUAc2jccmcHcmcHmHcmcNhkjCZbUAcZqhMXC2ptccptjc2jCpbtpcttjtjtjtpjjtpHL4pUyyHqCUUtpjpcckHUjODtcmjtpppUUUHktbpXNwccmHcAVvlVLyVlVvl;v;vvv:.,lC8eZZDCb29G9DGjcUkUrVM;                                                 
              vBZHGv;je0CCqppHkUmHcmcmcctctctctcjcckkUctptj20CqOGD0tm4XTXhUXUUk4UNUNUkcHHHmc0hwmCtcccmcmmkHkmkmm4AHtC@ChAmOb4rUj4TcbbtpcjcjtptjtjtppbpCjpjbttct4j#b4rhpjkXjtjt2bqbCtmkj0qtjtjckhUXkmCjUAwHjHmcwvvvvyUN4Ahylvv;v;:. :t69D9Zqmmq9O9O8B#UrywTv                                                 
              :6OpOU UeZCq0CptmcmcmtmccjtptjtjcpctHmccmjttcqq#qOD#pbcUhUXUXHUHUkUUNUhHccctmcckUmjtccHcUmmtjCtCj0ckHmHjjcUjjmh4Xkh4XHcCjpp2bbtbtjcjt22C2bjpccctcppCtcC9OGZ0tptpjZZZCChUjG#jcjtjmcHccpjcXUUkUUUcwLyVVkjttmHtwrll;;, .;VHpOOOCjH@DOZODO#04rMN;                                                 
              ,q8Z9b,ygZ#Cqpbpbjjjpmmmtj2tjtjtptjcjcjtbCZ0Z96ZbmmhX4UNUXk4kUUUkUk4UXkkcccHmHcmmHcccHcUX4bDqtccjmHkkNXm2pC2bj2jbj2tckb@#b0bC20jjtptpjbpbtpttHcmcmmHcmOO9OObjjpjptptjp2jpjjcjtjtbjbp20bhXc0N4XkUkmjjcwUtegG6e6Be9cA;:;,;tOOZZqC0OO99cyyNOcUN;                                                 
               wBO0#vvZ9q@CCpbt2tjccctcjtptpjjt2cjjptC@Z@DZGODckhUNkNUUUUHUkUkUk4U4UXkUUUkUHHcHcHcHcmkktj#HATNMXUcmmcbj2j2p0Cqb#Cq0Zq0b@q#b0C0ttcttjjCbCppmmHcccHccjttmtcpp2jbppjjjZ6OcwNjcttjt2pbjqjhAtbXTUhUXmmmMllUb@jbpbCO9ObHrv.;4@D9ZODOOOO6CU;;VMvyy                                                 
               .BG0#wvpZZCq2@@btcHjtptpjptjjpjpttcpcct#@DqZ2jXhTXNUXkXU4kUk4UUkUUwXwhTwrTwUUcHcHtmcccctctccctctcZDOD@jjcjtZ#Z#ZC0C#22tjtpccHtcmkmkmmj2C2ppptjjtmcmcHckHkccjtjtjtjtjZ8OtMktjcjtjtpt2pjrwcChNNUkHmtHt4XhXXHUUXXrrAHmcA;.lcOO9DOOGO9O0XYvry, yv                                                
                O8#ZcchmC@C0CDCtkUcbjptjtjtpjjtpcC20pZ9GODjckUwUUU4UXkUUXUXkUkUUUcHcmmHHHm4kHmHcHcmmmtmtcttpjbj2pjctmtctcjjbtCj2tjmcctcjctHcmppCC@CbttHHkmmcHmHmkHHmmtmcmttpctcmHmHctbjjj2jjt2j2tptpmHkttkUHktctcp2p#ZH4kHkkU4AXXmtHv;YtD9#DZODOD9CUvvAjY :;.                                               
                weBZO@r;j22C###tmXctptpjjjptjcjt2qDqZZO9OpmhHkHUHX4wUNkUkUUkmHcU20qbqb0b02jHHkcmcmcmmctmccjcjccmkN4UHmcctcc4XUHHjtjtjtCC0CCbCb0b#CCttUkXUUmUUkct2cjjC2Cb2pC2CtjmcHmHpj0CCjbjCj2jptbtcttmcmtmtctcjtmUCOCUmmcHckkUHH2my;ytO#Z0@@Z#ZZ#tUyYmjl,                                                 
                veG#q0wAj9OODOOOZ#bCjbj2j2jjtjctcZDZqZ@0mUXUkmmmUUNU4UUHUkkcmtcckkcq#CcmHccjtcUkUcmmHmHcmckH4kUHHttcU4wXhHkk4HktjbC#CCtHUhylvlvv;v;vvvvvvvvvvvvyVVYNUkUkmp2btbCbtjt2b@qqCCp0b0bC2bbbcjctttcccjcjtbjtqeZjcptpcckUUHtcv;ljC@CqqqCZ0Z#qHN;lXMvV;                                               
                ;qejCpjt9GeOOOOOOZDq@jbjpjpjptptbb0pbtmhNM44HkcUkNwTUUUUU4kmjtckcmtcjmcUHUHcbckh4kcmcHmkHUkUkUkkHUHUUNXhXNHHmHcmtcjmtmkvv;;,,,,...,.,,,,::;;;;;;;:;;;;;;vvVyyVhMkUtjq0@Cb2CCC2bpppCpptbjjcjtcHHUHkkUHkHmHkmUctpjtkjHyvVmb2qC00qq##OpHryyUrrrv                                               
                 bBCpjtDGg99OOOOZD000Cjjmtcpq9O6OjkkHHhTMXXUkmHHUNMNwUkHkHmcmtctt@bpHHkmHUhwwcjCcccjcmkmHmHHHmkkAMAhhkUHUHhTyLYYlyvvv;:.       ....,...,.::;:;:;:::;:;;;:;;vvv;v;vvLAXX4XHHcHmkH44wwATAMyryryyyyYl;;:::;;;;;;hmtHkkkXkctt2pC20Cq@@mHHHpOqmyl;                                               
                 L8Z@jpZg969GOeGg9Z0#CbmmHjCOO#CjNXNkUXrLyAUmmHUk4XwUH2jCjtmcmtmjpqmUUmmtmcHctqCtHcHmmjcm4kUkUXAryAyyllvvvvvvv;;v;. .                       .       ..... .,,..,,,:;vvvvVrArwXUUkUUrMAALALryLVLlYvlvvvvvv;vvvYAVrrlLj@@2pmtt22CjZCmHjtjCDjcXv:,                                             
                 .gGqqqOOG9e9e6eee#DDZ#O#GG9ZbHhA4XkX4TwAhNHmmHcmjtpjCjCpCttmmcmHm4kUHctjpcjttccXXwTAArhAAyVYylv;;;;;:....   ,,,,                                              ... ...,,;vvvvVrNXUhU4Uh4hXhUXTrTrwTUXk4NUHhMVVyyXHvvAZ9Ctjcbp#bb@qkHctTTylMkryv:                                            
                  Z88D@ZOGGGGGe6BGODGOOO69ObthrVAhU4U4kmtccmmHkHjjCpCjpjCtpctHtmcHHkkHcjbcchhwXALYVyy;vvv;,.,,.                                                           .     ..,,:.:,;;;;;;vvvvyHmXUUUNUXkAAAhTN4kUU4tmHrlvyVkHV;vAjkjtjb@2Cjq2jcbC@bXvVrwLy:                                            
                  ;0B9GGOOOGOGOG99cmHtcHU4UUXXATwmmmHmb#bjHmkmkccpjptjHmcbppttccmmHmHccbcjc4MUhUyvvv;..,,                                                                  .     ..,,,:;;;;;;;,,.,,lyMh4hNTwMwrArArTrhrXj2UylVlVhtXv:;vhc0qZ0CpCpqb2jbtkyvLU;;v;                                            
                   ,, Uee#DOODOD9#UANXkhXhUXkXHU4HpctctcjctctctctctccmUwcjDpcccctctctc2jtkwyVYrVlv;:. . .                                                           .....,:::,,.. .  .,.;vv;v;;:;,,  .;vLVLVryylVlVYv;:;TNHwNMTMktby;.,VUHj2DbCpCCCtpjjAvvH2yTmU;                                           
                      :B8899OOOg6GkUkHkHUkUkUUUkXmtcctHHmjp2tjtptpctmtUUHb0pktcptpjpttHHwrvv;;;;::,.                                                       ....:::;;:;:;;;;v;;;;;;::,,..;vvv;vlAVyv;.  :;vvvvvYVVyVyl;:;;vvYYylrUmUAvvvvVXcCCbj2CqqDZOZqb#0#jjbZ@U;                                         
                       weB8G9DO#DjUUHUHUHUk4U4kNUmptcHUhmjDZC2qbCjjcjmptCC#tccjttkcmtmXyYv;::,:..                                          .   ,;:;:,.,.,,,,;:;;;;;;;;v;v;vvvvvvv;;;;;;,;;vvvvT4kUmmXlv:;:;;;vyVLVVYv;;;;:;,;;vvTMNAYvlvyHCC@2C200OOODOjqC00jcpt0pV;.                                       
                        ;wt#OGZcMwNUUkXHUkUkUk4H4b@CckwUm0D9@0C#CCcptpt0CqjtmttthLrTALv;:;..  ..                                         ..,. .vvYvlvvvvvvvv;;;;;v;;;;;;;v;v;;;;;v;v;vvv;vvvvv;vlyN##pmhvvvryryyyyv;;v;;:;:;,;,;vvvAArvv;rmjtjtpbZZOOOOCpC2CHHHmtHvv.                                       
                          ;c99OHwhk4HUUUUXUUtmtHmpOjtNhXcpZ#cUmmcHtcjmHUmk4wXNHUY;vvv;,..                                               ,,;:,:;;v;;vvvlvlvvvvv;;;:;:;;;;;;;;v;v;v;v;v;v;vvlvvvv;vvMktpCtkMryrVrVyVlvvvvv;;;;;:;::,:;VMy;;;rNtctmmHtmcmjtjtpt2pCbZXvY;                                       
                          ;OBg#mwXXkXkXk4UNUcjHck2bjHHkHm2p0kXhUUHUctjUwhkTAyyvv:,.,..                                   .             ,;;;v;;;;;;;;vlYVllvvvlvvvvv;:;;;;;;;;;;;;;;;;;;;vvvvvvvvvVVrAHj#btUhyrVVYyVrrArV;;;;;;;;;;,:,vYrVYlycpmcmmmtmtmttjct0ZqZO0yyv                                       
                          ,c86ZtMMHUkUUUHkHHHUHmtttcjtpjCjpjtkcmcpZbZDHrMAALV;;,.   .      .,:;,,.,,.                                .,;;;;;;;;vLrAlrwkUUTU4UwXNUkmrYvvvv:,   .,,::,:,,.:;;:;;v;v;;vvV4MYyTNUAvvvvyyMAwrv;vvvvvvv;;;;;vYVlvvHtpttmttjctjbj2tOZZZG0wl;                                       
                           X8GbmAXUH4Hkttpctrr4tjptjt2p2j2jjmcHcpeeZtmVlvvvylv;;.. ...    ,;;;;;;;;;;, ...                           ,,;vvvvvvvTNkUHkttcHjttHkh4hUwwrTyrNy;:.:,..,::::,:,;:;:;;v;;vv;VXhvvrUHUyryyVyyyVylvlyYlvvvv;vvv;;;v;vVwkbCbtjccHmkHkcjC2C#D4v,                                       
                           k892hMMUUkUHHjcjkAAmtjcjcjtjtpppjbtjmptHV;:;vYYyYlvv;; ... .,:,;;vvvvvvvvl;;;;;.                         ..,;vvyVyyLvvvvvlvvvyrhMwrrrMrMMNXXwUHkAryv . ....:,:;;:;;vvv;vvvyHUrvLUmNkkHXLlyyyVyllvYvvvvvvvv;;:;:. vYAUbjjcjmmUUhhhU4mkcUL;                                        
                           MB6m4hkkmkHkcctcHHCZCmmkmHHkmHHk2q#cmAVvv,:vyrUml:;;;;;,:;;;vvv;vlVvYvvvlvvvv;v;;,:,,.,                  .:;vvYlyVyv,     .,v;v;YrXU4NNhHcMAjkcDOjcHXrlvv;:.,:;;;;v;v;v;;;vrMyllALyH0jcXwThyVvvvlvvvv;vvlvv;, ...,vYhHptpctkk4kUhLryrATl;                                        
                        .   8eHXccmkHkmHcctkcDGCmUHUHUHkmmmHmUhVv;vvvlMNwAhv;;v;v;;:;;v;v;vvvvvvvvv;vvvvYYVYlvl;;;:                 ;;vlVlvvvvv.  . ..:;vvvvlyXkHUXt6Y;qCNbO#mC##bp4Mlv,:,::;;;;;;;;:;vvrlVlVyUktpCttUNlvvllyYryArwTyvvv;:, .:vvvyjmcHkXXNUhALryAAy;.                                       
                 .     ..   B8mUmtkHUHkmHtmmHZ90kkmtmccmHttkyvvv;,:vVXkmAVl;;v;;;;:;:;;;;;;;;vvvlLyLLAMwXUwwAhMAvvv:.,..           ,;;vlllvv;vv, .....:vvvvY; vtmcNp0: kmym0myYAUctHMrVv,:,:,;;;;;;;;vvrrryyrUrrHZODpjkryVVyyArrrNUHALvylv,,,;;:.vUUhXhXwNw4NUXUXAyv;.                                      
                       .,   OeHhHmcHmHcptmkMUCObkkjtb2phUN4V;;;;;;vvrM4rYvv,,,::;;;;;:;;;,vvLAHmttCj2pjctHHTArMAMAhTTv,..          .;vvvlvvvv;;,.,,...:vvv;,  ;4UAMtGpqejvyVyv;;lVXUNLrv;;;:::;;;;;;;;vvrANAAcHvrZ6#ZC2UkhX44rryLAUHmUTrALyv;;;;;vArrlrMNMXXU4UXkXTrl;.   .                                 
                       .    vCUUNkcpttj@jtUwwpOZcHUHUHrlvvvv;v::;whhAAVvvv,,.,:;:;;;,,vNrwrmtjtpAryryLyylYlllVYyYVyAA;..         ..:;vvvvvvv;;.,.:,,:;;;;v:.  .yyVvYlVvYvvvvvvvvYAwUMAVvv;;;;;;;;;;;;;,:;yUXvAtA;wC9ZDCjUkkHUAyALAMUHmN4XMyY;.:;vv;vvlrTrNrTwNTXhkXwrl.                                     
          .         . . .   vbtUkHpbCjb0CtkAXt9DjwwMXAVvvvvv;;;;vj#HTLlvv;,:;:;;;::::vUmjcHmkAYvv;;;;;;;;;vvvvv;;:;;v;..         .::;;lvvvvvv;;.,,,:llv:,..,;;;;v;;:::;:;vvvvvvvrTHkkwrlv;;;v;;;v;;;;;;::vwY;,MHv;4OZ#ZjUwUUUrAAMAUNrMkUhLVvv  :;;;:;VNATrryryrrAUkA4hv               . .         .   ...   
     .             . . . .  ;pjkMkUmkmkjjjHmmmmcHyvyyylvv;;. .;vy#0kyy;;:,.....,:v;rHbckkcmUMy4A .lyVv,:,            ....        .,:;vvvvvvlvv,. ...,,     ..:.      :,::;vvvvlyrXXhyVvv;,.:,:,,,,:;;;;;,;vv:.vcv:VZ0O02cmmtHUhXh4k4yAktUMVVv;,,.. ;vywhAArryryryUmN4cw;     .     . .   .     .     . ...  
                    . . ..  .4#4XHUhUkmcpcpjjHmmtyYvVYyvv;:.:;vvMqCwLl;:,..   .,vyAtqjHUUwhc@;VOm4ZDCHAv;;; . .       ....       .:,;;v;;;v;v:.                      ,,,::;;;;vvvvv;;,,.,.. ......,...,.,,;;Y;;T4vvr09OC2cjbjkHkHUHX4hkcbkwLyVv:, .;;vAUHkm4NrrlyT4yUc4v    . . . .   . . . . . . .   . ... 
             .     . . . .   ljjtCHkHcmpt2jbtmkttrVyyArVv;.;rTyVl2jwyV;;,. ..:;vMbtCCtkmHUwpH ;OyvqpyhM, ,lv;;;;... ......       .,;:;;;;;;v;,                           . ....,.,.. . ,.,...,...,.,...,.;;vlY;vywVlte9qtjtCtHUHkkUUXUNHcHArLMlv;,,;:;vrMHmt4hyvvryrUjmhv,  .. ....... . . . . ... . . .....
. . . . . ... . . ..... .    ;HZO0pmcccHcmmkXrrrMvLXUv;.:;vybAVvVCCMrv;,,. .;vvvtZbmkAl;TjkmtcOZ: XcAwY:  :;;;v;,   ..,...      ..:;;;;;;;vv;:,....                                   ... ....,...,.,.,.,,;;vyY.vUUyt9DCZpCpjHHkHUkUUUHHtUUwrYlv;..,;;v;vLw4kwrlv;vYhUjcy,  ..............................,.
  .. . . . .     . . .       ;XZZ0pjtbjt4NNUyyyrLMAr;  ;vrUcjMvvV#chyl;;,, ,;vrNmbcUrr;.;UMNXUXcMvhXvlvv:.....       ,,,..     ..,;;;;;;;;;;,:,:,;,,                                         ........,.....;;yXvvwUMwmHqG9qjctHmmccjctctmmcp4rvv,. .;vYl,,vXUNAAl;,;vyHpl,  .......... .....................
, ... . ... . . ... . ... .  ;k#DbCb0Cbwk4mHrrwrTyl: .;lU4q#ChlvYj0UTl;:,. .;vATUmmyYlv ,VLlvvrl;,;;;;;.            ..,.,..     ,,;;;;;;;;:,,,,::::,                                        . ............,:;lmv;rkrNmt06D#jtcmHcHjtjttccmjcHyyvv: .vvrw; ,yUUUXkyv;:,vhNV: ................,...,.,.....,.,.
. ............ ....... . .   ;N#CCbCb#jwMUXmrwAyv,vlvylrHcpDjMlvvc0mrY;:,. ;;vlylArLyl;,;v;v;v;;:,...              ..,.,...    ..:;;;;;;;;::,:,:,,,,                                     .    .............:;y4V;Vh4XmmbGG@CttHckcctcjmckHkHkNhAllv,.;vVv;;vTcUNNHTyv;;YwkY. ..........,.,.,.,.,.,.,,,,,.,.,
. ... ................... .  :AmcHb20bHVyrMrryY;;vATMArXZpjChvVlvHZUrv;::..;;:;:;vLyrVv;v;v;vvv;.                 .,:,,....    .,,;;;;;;;:;:;::.,...,...,.                                        ......,.,,:vAvvVXX44ctGOZppmmHcHtmtmtmmHmH2j2hVAm, .YvAyv.vrkHcXkhMl:;rhk: ...... ,.,.....,.,.,.,.,.,.,,,.
. .,........................ ;rcHkHmkmhrrwUA;;vvlAMUNUMt9OctVvVyvkCHrV;:,. ......vVYvvvlvv;;,,::                 .,:,,.,..     ..:;;;;;;:;,:,,.......,,,......                                       . ..;,,.;vVYrhkUUkcZeDD@CjpjptjcptpjCpC0qjwvycw,:vrATv..;yc0bhUHU;;.,v; ,.,...,.,.,.,.,,,.,.,.,.,.,,,,,
, ,.,...,.,.,.,.,.,.,.,....  :htjUUAwMMhkAXV;;YyAyyyhXmb6DcXvvlvvTC4rl;;:.. ..   ;;;;;;;;;:   .                  .,,,.....      ,,;;;:;;;,,.,.......,,:.,.....,..                                 . .   ,::.,;vvyrUmmXUHZO9DOCpt2jtctcjjbjCj0pcVvvM02v .YAL;  ;AmmHUcHvv. :v,,:,,,,.,.,,,,,,,,,,,,,,,,:,,,,.
. ...,...,...,.,...,.,.... ..;vrMTrrrrAhVv:;y4rAyrNHmC@DO2UXvvvv;LXrvv;v;:.      .                             ....,,..,..     ..;;;;;:;,..,.,.. . .......   . . ... ...                                .:,,.;vYVAXmkk4t0Z#O#0tptjtjmtcjp2pCpCtkwrMC@M  ;ywM::;vYHUHkHAV.  ;.:,:,,.,,,,,,,,:,,.,,,,,,,,:,:.,
: ,...,...,.,.,.,,,,,.,.,...:::;yryrhyr;;vvVMrrYlMct0Z9O0kUAvvYlvAmyyv;..                                      .....,...,.     .::;;;;;;;..,,.. .   ..... ... ..... .                               . ..:,,.:;vvyrMhmmct@0@CCpCjpjpccHtcbC0bqCjmUhUccy, ;rtqjXL;,rckUUchl;.  .,,,.,,,.,,,,:,:,:,,,:,:,,,:,,,
,.,,.,.,.,,,,,.,.,,,.,.,,:,,,;:;vVLAVv;;;lyryyyryHCZZDO9jUALvvvvvXhyvv                                         ,...,.....,     ::::;;;;;;, ..,.  .   ......... ...                                 ... ,::.,:vvvyAr4tjcbCqCq2CpbjptjHcccpZ0qq0ckhkpDqX..vNc0CcAv:ymUXkkckTr   .,,,,:,,,:,:,:,:,:,:::,:,;,;,:
, ,,,,,.,.,.,.,.,,,.,.,,,,,.::;;vvylvv;rUMTVryhXmZGOO@DDjMlvv;;,,vv;;.                                      . ..,.,...,.,.     ::,,,;::;;,. . ,       .  ..           .                             .. .;,,.;;vvvVAhHH2jC2CbCpCjjjptcmcm200pZObTwkqZ#mwvv;vyjCwvvrtUHUkUUmZ.  .,,,:,:,:,:,:,:,:,:,:,:,:,:,:,
,.,,.,.,.,,,,,,,,,,,,,.:,,,,,::;vv;YlrYXXryLrUkc@OZD#OOGphvvvv;:,vvv;:                                     ... ,.,.....,..     :,,,::;;;;:.,.,:          .         . ............. .               ... ,::,,;;vv;vyThcp0jjc2jppbj2pCttUhLUHtq8CryHtcHjqM;..vcDhvv;vcck4U4m#v  v;.:,:,:,;,:,:,:,:,:,:,:,:,:,:
;.,,,,,,:,:,:::::,:,:::::,:,:,;;vvYVAMTAUVLAmcb2D#ZqZ#OtmAyvlv;.,vlv;:                                      . ..,...,...,.     .,.,,;;;;;:;,::;           .     . . . ........,......              .. .,;:;:;;vvvvVyUm0bjmttbjbj2b0bCjHMrAkm98@yyUtHttbwv,,vpZhvY;;ycHHUkkbH  vv,,,,:,;:;:;:;:;:;:;:;:;:;::,
,.,,,,,:,:,:,:,:,:,:,:::,:,::;vvVMAwrMrUX4hkb@q#CqCqbDjvvrArvv;..ll;;;                                 .   ......,.,.,.,.,     ,,,.,:;;;;;:;,:;,..                     . . . ..... .               ... :;;:;;vvvvvvMkjCqcjtjtb2bjCb0ppHXT4Hc28CrAtttHHUUVv:vMUVyVv;;r#Cc4Hkcb, ;;.,:,;:;:;::::,;:;:;:;:;:;::
:.,,,,,,:,:::::,;:;,;:;:;:;.;vlyTMArArMUUh4Xbq#CqC#CqZjlyrwrv;;.,vY;;,                                  .   ......,.,.,.. .   ..:..,:;;:;:;;;;;;;.                        . . ... . . .           .....,;;;:;;vvvvvVUHpjtctccmccp22jtUXrwMXrwDbrm00tcXHXAv;vylYVyvv;vymkkkkMOG  :;:,;:;:;:;:;:;:;:;:;:;;;:;:
:.,,,,,:,:,;,;;;:;:;:;:;:;:,:YlAhXLAyArUU4NNpZCq0#@Z2DmVVAAyv;:,,Yv;,;                                   .   . ..,.,...,.        ..,,:,;:;;;;;;;;, .                         . . .   . . .    .....;,:,;;;;;;vvvvv;;VhUtccmckUkpq@p2mUAMAUwMTOpAkZ2jk4kmwY;;yyYVLHMv.:kckHU4re#  ;:;:;:;:;:;;;;;;;;;;;;;;;:;
;.;:;:::;:;:;:;:;:;:;:;;;:;,;rATkMyVArhNkHtc#qqCqcckctkVrrwyv;;.:vv:,,                          . .            .,.,..               :,::;:;;;;;;;;;;,                         . . ... . . .   ... ,;;,,:;;;;;vvvvvvvrwmtcmttmUctZ0CjcNAAXh4TXC2NmZqtmhkk4VvvlyyvwtHv..HkHkHXyUBA .,.,,;;;;;;;;;;;:;;;;;;;;;;
;,:::;:;:;;;:;:;:;;;;;:;:;::;4NwwrvVr4NXXHjq0@qZ@cUmHjXLrMry;;:,,v;:,,                                        .,.,,:.              .,:,,,,,,,:,:;;;;;:..                       . . . .   ..   .....;:;,;;v;;;vvvvv;vLUcUActkhkkpb0b2mUMwwUNhTUUttOGOpc44wUmV.MHVwUyv..vyXctXNcHZv.:::;:;:;;;;;;;;;;;;;;;;;;;
;,;;;;;;;;;;;;;;;;;;;;;;;;;,vwhrTyVYThUhkkjbqCDZOmHmttkLrrMlv;;..;;,:,                                        ..,,,..               :,,.,...,.,,:,;;;;;:.                         . . ... ....:,,.;;;;;;v;;;;vvvvvvvVyNMhjCMANkt02bjtUhT4XUww4kcj09ZCckMhNcA ;ZhwkTl;. :rktUHHhmc:;;;;;;;;;;;;;;;;;;v;v;v;;;
;:;;;;;;;;;;;;;;;;;;;;;;;;::;MVVYyyrrTTXUccCC@@#@tHHwwTyVhwVvry; ,::,:                                        .,.,,.               ..,..       ....:;;;;;                            ... . ....,..:v;;;;;v;v;vvvvvvvYyMNhppwAUUtb0j2mkTww4hhMHkjtZOD2t4NTNUm..0jM0mhYv .vXHtkUUHk;;;;;;;;;;;;;;;;;;;;;;;;v;;
;,;;;;;;;;;;;;;;;;;;;;;;;;;:vyyvvYLyryAwUH2b00q2CHm4MywYyXwvVrA;.,;:::.                                       ,....               ..,.;;lvvlVy;  .,.;;;;v.                        . . ............;;;;;;vvv;v;vvvvvvyLXMhpCAMXUt0b2tt4MA4UcNUkcmcCO#ZDchHwcjv:hCqbCjckM, .wcHNUkH;;;;;;;;;;;;;v;v;;;;;;;;;;;
;;;;;v;;;v;;;;;;;;;;;;;;:;;vyyvlvyrALyyNUcpq0#CbjtmkMrAyVHrvlAV; ,;:,;.                                      ..,.. .,;,.         ..:,:;LH20G@D2; .  ,;;;;:                         . ....,...:,,.,;;;v;;;;;vvvvvvvvVyw4XrjCkrUUtb0tchMLhtqcTUmmHUCZDqOcNNr4ml;hZ@CH4rMmX; .tkXTkUv,;;v;;;v;v;;;v;;;v;vvvvv;v
;;v;;;;;v;v;v;v;v;;;;;;:;;vlAVlVVVhTMyrMHcCCCpb2CtmNryNYyHTvYrX; .:,;;,                                      ....:LhXy:       ..,,,:;vhb99eOCjCh;.  ..:;v;;.                        ... ..,.,,:,,,;;v;;;;;;;vvvvvvvVrTUhhcqUMNkcqCjkNrywbCUykmmUkCO0#qCjcVvyy;NZjNwVlvrmU, rcUUUUv:,;;v;;;v;v;v;vvv;vvvvvvvv
;;;;;;;v;;;v;v;v;v;;;;;;;;vVLVvyYLTwrALkHjC#tjtjtjkUrAMyykAVvrky,,,::;....                                   ....vMtpCUY;. ....,,, .;rhcHctkAMr4v    ,,;vv;;.                      ..........:,:,:;;;;;;;v;;vvvvvvvyrUUUhctUw4Utb#tkMLyUtqXwmkUHUt2Z@OZOC4vyy;y02tctcc4cjk,vUUhhAv:,:v;;;;;v;;;vvv;vvv;vvvvv
v;vvvvvvvvvvv;vvvvv;;;;;;vlywVlVYVArAyMktjOZtmccjkMMkMMVrwMVVy4l,,,.::. ..                                  ....,::;h#OCby;,:;;;;,::vVyvvyUyYvvyv    .,;vvv,                        ....,,:,:::::,;;;;;;;;;;vvvvvvVywUHkHkmUU4Hc@@tAVyhUtj0qchUUXhUcC#ZDGpryU;yt0tjjptccbUv:yyNTUAy.,;v;;vvvvvvvvvvvvvlvlvvv
v;vvvvvvvvvvvvvvvv;;;;;;;lVTylvLVLrTAwAkcqDOcmHccUvAHMVLyryrVTTv,,,::;.  .                                 ..... ..,vr4qG6mrvv;;:;:;;;.    .        .,,;;;;:.                      .....,:,::;,::;:;:;:;;;;;vvvvvlYAwHHmmmUkUkUp@DcyvLNHH2ZOcUUmUwyNUjpOOjlyMvlm2jtjj2mtck; ;HUv vMv;lvv;vvlvvvvvvvvvlvlvvvv
v;vvvvvvvvvvvvvvvv;;;;;;vvyM;;lyArTA4kmcbjZ2HmCcHwywmLyrMLrAUNAvlvv;vv,....                                  .,.        ,v:.:,;;;;;;;,.            .;:;,,.,.... ..                  ....,.,,,,;,:,;;;;;;;;;;vvvvvvrLXktmmkHkkXHc@ZcVlyUHmjD#jUmHHArMcct0OCUyMyrNjtpjCckXkmy ,c4  VCv ;Vvv;;;;;vvvvvvvvvvvvvv
vvvvvvvvvvvvvv;v;v;;;;;;;vvv;vlLrAyrMHtbjjjbXmq#kNyrTrAN4UhU4Xyvv;;;vv,..,.                                  .,:              ,;;;;;;;,,  ...      ::;:,.,.,...... .                 ....,.,,::;,;:;;;;;;;;;vvvlvvlrAkcCUwcjwU4mkt0mVrXkkb2ZcHmtmwywcCHj#O2wrwATmCjbpkwXXmN:.;: .HbA   .Um   VylvlvllVlVlVYV
lvlvvvvvvvvvv;;;;;;;;;;;vvv;vvYyAryyw4cjbjpjUh@bkryyryMXk4UhXhwyv:v;v;. ...                                  .:.,..                ...    . . .    .,,,.,.,.,.. . ...........       . ..,.,.::;:::;;;;;;;;;;vvvvlvVrN42bNrbtXXkUXU6CLLUUUtqZjHcmcUwMUmUj9ZpAMMwrcCbjCkhMXkkv: ,YkUThm,  bZpUbbAvVVLyyyLVyyry
vvvvvvvvvvvv;v;;;;;;:;:;;vvvvvvyrMM4hUUpp2tjUmbpArryVLAUNXXUNUUHY;lLvv.,,:,,.. .                             . .,,,,,...                       . . ... ......... . ....,.... ... . ......:,,,;::,;:;;;:;;;;;vvvvvYVATkjphTjpXHUkhmOjVAkkXjbOtmmtmjtHMLwtqZjUT4ThHjcccC0bmHXr;  jGeDbr;w8Ojjq#jyvvYYyYyVVVyVV
vvvvvvv;;;;;;;;;;:;:;,:.:;vvvvllATNX4hXmbtjcmc0tATUvvyXX4hXhU4UUrvMAyv.,:::.. .                                 ,...,                       . . . ....,.................................,.,.:,::;:;;;;;;;;;;vvvvvvvVr4ppHAkUHkmkkktHUhkUUmCZtHcctcptHhkmc@GbUUkUmmtkmp9@CjcMw. lZBBCY yBDbbUTNAVrrALArALMrMA
vvvv;v;;:;;;:;;;::,:,,,:,;;vvvvYLwTNhNAtttctkcppVwXv;hHUMNhXUUXHwVX0cy...:,.                                   .......                         .   . . . ........,...,.................,.,.,,,,::;;;;;;;;;;;;vvvvvvvyUt0kyykjjmmHcHctjmhAUt0ctctmcctcjmcmZGCUUUmkcHHHmHtCGpUM. v@9ccctcCkb@jTTyrrMrTAMrMrMAT
yvv;vvv;;;;;;;;;:,,,,.,,:;vvvvYYAMUNUwXmjccmccpcUUMvvhjUwwUhkkHkHvyOe4:,:,,.. ..                              .......                                   . ..,.........,...........,,,,,,,.,,:,:,;;;;;;;;v;;;;vvvv;vvLUCpHyrUbccHHmcmcctT4UjjtmcHmmcmtctHt09pm4k4kHcmtkUkp0bHU: ;0jUtq9Zw,VDcwyVyALArALrLAAwA
v;;;;;;;::::,:,:,:.,...,,;;vvvlrMXhUhXNmccmcmtctmcrlvrccAMwUUHkHmNvUg#v;;;,, ..,                          ...,......   . .                                   ... ..,...,.,.,......,:,,,:,,,,,:,:,;:;;;;;;;;;;vvvvvvvVMUHNhhUkkUHkHHtmcHHkHkHHmkcHtcjttcjtppCcHNXUmHtcmHcHmpbHv .UZHjtbCOqjk4rrVArTAwwhwhwhMN
;,:.,,,.,.,.,.,.,.,.,.,,::vvlvyAhhXhUwkktccmmmtcjtrvvytkNMNhHHmkckVyOOy;;;:.,.,..                       ..,.......... . .                                 . . ..........,.,,..... ,::,::;:;;;:;::,;;;;;;;;;;;;vvvvvvyyh4kUkXUNUXUXcctmHkkUkkmmcHmHmmmHjjCcp0bmXTkmcHcHcUmmjbCwv lZ#90cHUHUUrhTXhXTXMXN4hUhXT
;;:::;,,,,,,.,.,.,.,.,,,,;;vvvvAA4wXwhUHmcmcHmcttjrv;TkkX4w44mUHHtyvm9wv;;.....,                        .... .......                 . .   .   ...       ......,.....,.,.,,,.,.. .,:,,,;:;;;:;:;,::;;;;;;;;;;;vvvvvlYAXHUkUkNUhXXk4HmHkH4HkHUcmcmmHckmtbjct0pcwNUtmmHmHHmtcCCX; ;0OejUyArrrwT4NXThMNTXw4NUhX
;.:,:,,,:,,,,.:,,,,,,,,.::vvlvYyXNkUU4UUmkmkmmcm2jL;YcUTUhUXU4UXkHhvhDOUy;  ..,..                       ...........                         .     ......... .   . ......,,,,,.,.,.,;;:;;;:;;;:::::;;;;;;;;;;;;vvv;vvvYyhHUHUkUk4UUkXUNU4mmm4HmmHmHcmcHtmcmpjCpj4TUkXctjUUcqj#M; .rZZkv;yMyrThMwMTATANMhAwTNh
:.,,,,,:,:,:,:,,,,,:,:,:,;;vvYlAhUUHUkUkHHkHkmmcccl;ljTAwUwUXUX4Nc4yVkO9hv,..,.:   .                 . ......... . .                 .   . . . ....,,;::,;:;,:,,.,.....,.,,:,,....,;;;:;;;:;:;;;,;;;;;;;;;;;;vvvvvvvvvvMmmHHUkUkUkNXhNrAwXMhmcmcHmmcctctHHc2C9OmVhXkc#bUAjZDGjL; ;UpU;;UcXrNThMhAwMTANMwrNMw
;.,.,.,.,.,.,,,.,.,,:,,.,,;;YVvyU4UkkUkUHUHkcmtkHwy:;UtcchNhXhUhXC0rYYbe0v;,.,,.. ...                   ......    .         .  .....,,:,:,::;;;;;;;;;;;;;;vvvvvvvvv:....:,;::,,.,.,;;:;;v;;,.  .,:;;;;;;v;;;v;v;vvvvvvvXtHk4kUkUk4XhkUhlvvvybttHcmcctccHckcjZGGmyAUXtq0hAm96epMv  TbU:;4cwTwhMNwwAhATThwNTNM
;,:;,:,:,:,:::::,;;;;;::,;;vA4VlrkkcHmkHkkkHmjcUrXy;;UbCtkwNMNhXwmkhMwL0Gk;:.,,,...,                 .........           .:;;;,,.,.,.:,:,,,:,::;;;;;;;;;;;;v;vvvvvvv;vvv;:,:.,.,.,,;;;;;vv;.    .:;;;;;;;v;;;v;v;;vlvvvwHHHHkHUHHUyAkjmwYl;vhHkttjcjctmttjtttO9HYrTkHptHXccDZjAv.,rCMvvwhMrXwTAwrMrArwATAMMw
;:;;;;;;;;;;v;v;v;v;v;;;YyANkUAvYrUkmHmkcXkktttkwAT;:NCtjkUThThTNwNwUhVr#Dr,..:,,.....               .:,,......     :;;;;;;:;;;:;;;:,.:,:;vvv;v;v;vvvvv;vvv;;vlVyVyrwATMV;;;;:....;;;:;;vv:.    .,;;;;;;;;;;;;;;;;vvvv;yNUtccmtct4v:rtqmXyYllyAkjjbjjtjtpjpmH@6NlyT4mcjcjmHH9OU;:vrrATArArTMMAMATAMAMrArMAAL
;:;;;;;;;;;;;;;;;;;vvvvrXkkHUmUMVyMHUUUHHk4kmptHTAwy r0ccccUANTNw4NXhUVVUZN;,;,,.. .       ..  .     ,,:,,.,.......:;v;v;;;;;;;vvvvvvvvlvvvYvlvvvvvvvVvvvlvvvvA4XUNUhk4kUkUmcw;:...::;:;vv,     .;;;;;;;;v;v;;;;;;vvvvvrTUctHcmtpU, ;Xt2tUw4wTyVVHcjj2jpj2jcUO6kvyAUHtcjccHbg94Yvvv;vwAMyArhAArMrArTAMAMAAAM
v:;;;;;;;;;;;;;;vvvvvVTmjccmcHtUrlr4Uw4UkkkmmttUhywl yCmcHthhhNh4XUXU4XrTXHrv;;,,....     . . . ... .,;,,,,....,:,,:;;v;vvllyvlvvvvvyYVllv;;;::,,,:,:::,;vvvlvlVLyryrrArAAwhkwyvv;;;;:;;;;.     .,;;;;;;v;;;;;;;;;vvvvvvVhccjt2tOH, vvyk0jHhUhyv;;VNjC2tptpckbGHyyhXHctcjtt2O#qkl, .vAAATwhTNTMrArMrMrArArwM
v;vvvvvvvvvvvvvYvvvVAUkmHHUmkcmHLVy4NXhkHcmjcjckAyXr;AcjHcckhXXUUUUk4kwTMhXAv;,:,:::,,.,.,.... ... ..,,:,...   v;v;v;vvvvv;;;;;;;:::,,.,.... . . ..,.,..:Yvvvvvvvvvvvvvv;;:;;vyTyv;;,:,:;;.     .:;v;v;;;;;;;;;;;;;vvvvvvMHjjbp0DH .;vvUbbU4XHkHAVyTXjpptjjC4pOph4hUNkcptpttXm0cy;.:lyyAwHXXMNhXNXhUh4MhMhTN
v;vvvvvvvvvvlvlvvvYyM4HUk4UN4Umk4rTAMrMTjccmtjtNwLkylMmccHcUXNUXUUkXUXhAMMXrv;:.,,:,,,....,.,,,.....,,:,,..:vvvYVVVVlvvvv,  . .                  .,,:,,,;;;;;;;;;;;;v;v;;;;;;::,:,. . ..,.      ,;vvvvv;;;;;;;;;;;vvvvvvvykcCbbp9U.,vvVH0c4AXUjCCHXTNhtjptCCHj9cwNk4Ukb22ccUTyhXUyv;YYlyhUUwNTNMhwwTNAhANMMr
v;vvvvvvvvvlvlvlvlvvVwhkXUX4hUUHUUTAyLVrmpmmcjckTN4rY4mtHcmHXUXUXUUkUkUkNXXwyv;:,:,,...,...:,:,,.,...;;:.:;yANryvlvy;,.:,.....                   ,,;::,:;;;;;;;;;;;vvvvv;;;;;;...:.    .       .:;;v;v;v;;;;;;;;;;vvvvvvvywcCqj0ObLyvvVCqCUNNUUcmmHmkccjtttjUjZCXXh4UmmjcjcmHHHttHlv;vYN44wNAhMNA4TwAMATAMMN
v;vvvvvvlvlvYlVlYllvrAU4UX4hUXkkHhNAryryAmtHcm2pkUTvyktcccckUhXNUUmUHkmUHkkMrv;,,,,,,,,.,.,,,,,.,.,..,;;;lUHctkLvvvvv,,:;::....                  .,.,.,.:;;;;;;;v;;;vvvvv;;;;;;.,,: . .       ,;vvv;v;vv;;;;;;;;;;vvvvvvVrUmDpcNht@kMywpOqmXUXUUkkmccHmHcHmkHH0GZAyTmkUk2C2ctctc2twv:.H#tUtkHmmkcHcmcmcHcHmk
vvvvvvvYlylYYVVVlVllyUUk4XXUNUhU4UMArryrAtttmtjCUUNrwmctmtmc44wU4HHmkmHcmmHmwrv;.:,,,:,,,:,,,,...,...;;vrUXkXkAyvvvvv;;;;v;;:;;,.,..     . ..      ......:;;;;;;;;;vvvvv;;;;;;.,,;,. .        .;vv;v;v;v;;;;;;;;;;vvvvvllANpbmXlvHCbHULNjqkUUU4UkUkcHmHmkmkUkmjG#MvNHkXH2qtcmccjjbAv. y#tjbq@Z##C#0qC@C@C02p
v;vvvvvlYvllVYYlVlvvrTUhUhUXXwXh4hNAALrrMUcctcjmUktttcptpcjjkwXXUUHkHkmkHmjjmXAv,,:,:::,,,... ...., .vVyryVvlvvvvvvvv;;;;;;:;;;,. ......,,;;;;;:;;;;;:;:::;;;;;;;;;;;;;;;;;;;:,,;,:.  .      .;;Yvv;;;vvv:;:;;;;;;vvvvvvVAUjtrl;v4CjjHyvVNkXkhXh4wHmcHmmmHmkmm2DDkMwHN4kbttHmctj@bA;. vZODOZ9D9O9O9DOOOZODOO
. ;v;;:vllvvvVvVlVvvVrMwhUhXwXwNMhTALArAyAXcc2ckXHmjHctbtjtpXwhXXkUHUHmHMjb0tcUU,.:;::::..   . ......vVylv:;,;:;;;;v;;;;;;;;:;:,.. .,;:;;lLAVMVYvylvvvvYv;:;;;;;;;;;;;;;;;;;;;...,..  ..    ,;;llyvv;;;;;;,::;;;;vvvvvvlVhXcUy;;vHpjmtwLlLrhhXNNAM4tmmp#bqjjmtcc@6jUTXTUmjmUkjj#G@Av. vBB9ZDZODOOOO9O99GOG9G
. ;:;:;vYvvvllYllYvvyyMAUXXhNwhNNwhrrrMyrAUcjccUkUmHHUmcjctcUM4hUXckHHpNVc9jHkHcv ,,:,;,.       .....   ....,.,,:,;:;:;;;:;:;;;;vvyyArAAhhkUkUkXUkkwwAwryYv;;;;:,.,;;;;;;;;;v:... . . .     .:;vlll;;;;;;::,;;;;;;vvvvlvyMkmy;;vrUcktcbcUrVYyrUNXrccckC#qbbjjccUC9jNhMMwtjcMkcpC6bHkl ,p8OOZOZODOOG9G9G9G99O
. ;;;;:vvlvvvYvYvVvvlyrMhUh4wXwUhhwTrALrANUHmcHHkHXHUkkmctctXwh4XkHmUmpwlt9C4Hktv  ,.::.                 ..,,,:;;;;;;;;;:;;;;;;;;lywhUXUMrvVVrXcmcckNUXhyLv;;;,,...:;v;;;;;;;: ..... ......,;vvvvvvv;;;;;;,::;;;;;vlvvvyrAhhvv;VrhNHc0#Z2c4UyyLAyMXkXUtbjCjjccmmpGpHwwr4cjH4Hjc2qbmCpv.;yZ9OZDZOO9OGO9OG99O9
. ;:;,:vlvvvYYyVYYylyVLrXNUhUX4XXwNAAyUmcHcHkkcHmkkkHUcmtccmUMUhUhAwUcbhVUG0kUctA;;.,:;:.             , ..::;,;;;;;;;;;;;;;;;;v;vvLryvlvvvvvvvyVlyLVylyyryv;;:,.,,,,;;;;:,,,,,,.. . . ..:,;;lVvvv;;;;;;;:::,;;v;;vlvYvyMMrwyvvYlrTUktjZ0244NkX4MLANMwhctb#OqCjpj0D6OtwTNkUctptjctmttZc; .49Z#ZDZ9OOZ9DG99O9D
..:;;;;vvlvVYVVyVyVyYVYANUNXhUNUNNAMLrUbtjttmtccHmkHkmmtctmHNNNNMAyw4t2UvhGO4HHjM;;,.,:;.                  ... ,:;::,,,,.. ....;;;;;vvvv;;;;;;;;;;:,,,,;;v;;;;,,.,.,,;:,,......,.... . ,.,,,vlvvvv;;;;;:.;,,;vv;vlvlvvyMrAryvlvVVTUmcCq@pHUkkmcckk4Uwkcp09gZtbbCjO6gpUAUNUcCtjtjcjmjbc: .tODqDDOZ9DOD9O9OOOO
;:vvlvvvvvvvvvVlVYLVyYyrUXUwUXXhNwwAAVktjcjHcmcmmHHkHkccjmtkXTMrTArrkj2NVrDZHHHHrvV;. :;;:.                 . ..,,,.... . ......,.,:;vv;;:;::.      ... ,,:::,,,,.,.,,,,,.,.,...,,:,,.:;;;v;vvvvv;;;;;;:,::.;vv;vvYllvrrALAVylYvyAXkjjD@bHkUTMHcp2pctmtt#O60jjCjpZ6OtAMhUhcpptjttctcCHv .4OO9@ZZODODOOOO9OGO
yyrhTwrArryryrLryArMLArAwUMUXUTAyrrAXHmtcjtmUkUmkHUHkHUHUHHkwTrhAALywpkwMwcCcmNNywhA, ,;:;,.               . . ,,:,,.,....         .,;::.. .            .;;;,,,,.,.,,,,,.,.,.,.,.::;;;;vlVlYvvvvvv:;;;;;:;::;lvvvYlylYyryryyyyVVYAXmj2cjmckUyMUtcjcckmmt2O9ZjbCqbOZO@th4hkHjctmcmtctcjm; ;C69CcD@ZqZ#OZD#DZ9
hrk4U44TNANAwAAAwAwMTrMAhMhXHUAlYYrrkmjcctjkUXXNU4HHHkkUU4UwAyyVrrkHjcwrNUbjpHkwyTchv.,,:;v;,.                . ..,..   .                                .,,;:,.... ... . ..:.,.,.,,;::;YvlvlvvvY;;:;;;:::;:vlYvvvVlllyyLyyyryrVrAUmjjjHcmc4LyUccTrVTwmcp0D@#C@q##ZOeZmNUmtmcHcmcHmHcjc; ;#geCj##q#q#qZqZqD#
UhXhhUAwrMAwATAMAMrMLAAwTNAXkHAYvVyMUcttmjtch4N4NkkHUkUkUU4NyyVylAcZ#cAArp22tckNlwptv;,. ;;v;:,                ..,..     .       .                         ,;;.. ..        ,,:::,:;;;;:vvlvlvvvvlv:;;;;;,;,,vlvvvllYlVyryyyLyryLrMXHcbcmkctXvLhjkLvYhkktjOZZ0q0Z0OZ9GqNNwkmtHcHcHcHccbA: vqB6OqDZZZZZO#OZOO9
kMUwhTwArrAyryyyyyrLALrrMAwTkAVVrLwTmcpttcjctctccXk4kXXMmpcMyyLyVAtZ#4LrUjpttXAyVUO2hl;  .;;;vv:              ..,,,..           .                         .,;;. ...        ,:;;;;:;;v;;vlvvvlvvvvv;:;;;,::;;vlYvvvVVyVrrArrLALrVANHkmmckmcclvymjmYvympCtCbqbqCqp#OOZGDCH4wUHcmmHmHjcp0A.  ;yjCqCCjCjb22jbjq0
4NXNMhAAyLyLVyVVYyVryyyAMwrhUy;rUTAUmtcjcjtjjbj0jH4UNkUTUZcMlyyrl4t0p4ywcjctUhlvYcG9jw;.  ..;yyv;;,,..         ..,.,.                                      ,:;.....        ,:;:;:;:;v;;vvlvvvvvvvv;::;::,;;vvVvvvlVylLrMrMrMrAyyymHmkckcmckvvykCHYvAt0C0pbjb2Cppq6DO9gOqmwMHmcUjppHUhcHTv, vHCccj2jptjtptjmc
HhUATMwrryryLyyVyyrrMrrrwTAAUyvyUMMwcmptpj2tjjjjbtthMkbhTUUAllrA4HjmjXMmCkUHjrvvvheGOUAv;;.  .;lAVv;;;:.. ...........           .                          .:,, ...        .;;;;;:;;v;vvvvvvvvvvvvv;;;;;vvvvvvVVMAMvvlMMMTMrryrYrmj4hrkmcHmvvVHjcLVAHj0bCjpj0b2j#9OO6OOjHANhhNUt#jt4NNmth;.:M4XU@qqbC2Cjbjbp
VvYVvlvVYYYYvlvlvYVyVyVyLhTXhMVrwrhjtjcjjptppbjbpCjNLmbUMk4wvlyhkccHcUMpjkyU@A;vvkGe@jmAv;:,.  ;vlvlYYvv;,.,.... ...             .                         .,:.,...        ,,;;;:;;vvvvvvvvvvvvvvv;;;;;vlYvvvvvrUkAv;ywUMhMrVLyAwCcNvvktmcHyvywkU4rNwH2bj2cCCqjb09ZO96D#tkNwrhwcCbcptHNyvv;  vvrkcHmHmctmccj
VvYvlvvvvvlvlvlvvvvvvvvYrAkXNAMATYU00mctCtptbjpjbtjhwHCwNUkAlvyNHkmkHwhtbHrcZV;vYHe9qpCkNyv;:   .;vlAryv;...,.. ...             .     .                    .,,.....        ,,:;;;:;vlvvvvvlvvvvvvvv;;;;;vvvvvvvyUNTvvVwhTAwylVrXcCjMlvctjctkTyyVrhXwhwpjtcct#C02#OOZGGgOGCmyrwhH@jjcjtHyVlv .vlvyVrLrrXhNMhM
lvvYvlvlvlvlvlvlvlvlvvvYYhkHTMAhryUqbccjttcpjbtptjmmHcXArMLX4kkHUkUHUUhcmcMZZ;.vl0eeDZpmNMvv;;,   .;;VYv;:,,.,.. ...                 . . . .             ..,,,   .         ,,;;;;;;vvvvvvvvlvv;vvvvvvvvlYyYyYrLwXAVvvyyhrwALvyrXmbmkAyrUHttjH4VvvwhNAXjbtjcb0qC@qOZODG99O0kwANAkjpmttppHhwv  vvvvvvvlyyryLVy
lvvvvvvvvvvvvvvvvvvvv;vVAhUAMrMyLyHjtmtctcjtppbjbmcUmHXMArrwcHmUkUkUkUkUHhkO0.,vy0BeeO0tm4Avv;;.   ,;vrlv;;.,,,....                 .     .              .,.,..  .        ,,;;vvv;vllvYvlvVvvvvvvvvlVvVLALrrTAwhUrVlllVrUUXVVVAXmccctl:;rNc#0tkLyyyyrTctcHmp#b0C#qDZDDOqbckAhwNhcccmcjZcrv;..;vvvvvvvvvvvvvv
vvvvvvvvvvvvvvvvvv;;;;;AHmyYyArrVMcpmccjjbcjtbjpjmHkHmXhrMyhXkUk4kUkUkktkXtGU ;vLbeeG6OqmkMrvvvv.  ..;vvvv;;.,......                   . .           ......,.. . .       .:;;;;v;;vlvlvlvVvVvvvvvvvvvvvVyryyrAAhUTlYvVYrmjkyvyrXUHHcjw:;vVytbcXwLAyyyNHcUHHCC@C#@#qD@O@bHHMhAXhNHtmcc2ch;;;..vvv;v;;;vvvvv;v
v;vvvvvvvvvvvvvvvv;;;,vrcHrvyyryyACtHHtcpctcpjptjtc4UXNMATUUmHckHkmkHkccHXjOM ;vXqegG6B90HUhylVv;:,   ,;vYl;..,.,.,..                  .    . ......,,,,,...,.. ... ....,,;;v;vv;;vvYvvlYVyVYvvvlvvvlvvyTrMrAAwNkAYvlVyyHmkYllAhkUcc2myv: ;YHHcchYyVLMmmtmt20jCC@0Zq#DZtck4MwTXTcccmjtcXr;;.,;vvvvv;vvvvvvvv
v;vvvvvvvvvvvvvvvv;;;;vA4UryyrLrlwjtktcpctmtt2jbjCUNyyyrAmjb2jttmmkmkmUUXkb0;.;ymGeBOOOO99ZChV;;;;;;;..,:vv;:;;;,:,,...,.          . . . ..,.... . ..,.,,,,,.,.,,,.... ..;;vvlllvvvLyVvyVryLyryryLyAArLMhkkkAwMwwMlVlylrkjUrlyAXXcmccZqU;..vyNHjNrALYrrXmpb2j2cpjbC0CD#jHHwXAhTXcjmcctt2ty;;:;;v;v;v;vvvvvvv
v;vvvvvvvvvvv;v;v;;;;,vThAMyryryVyjtcHmmcHmcC2CCqtkNAVryhHpt2tjccmcHc4yAk@OL:,lkmDB69#OO99BOtrv;;;;;v;:...:;vvv:,,,.......          . . . . ......,.,.,.,.,.,.,.,.. .   :,;;vvlvvvlyrYyVyVLVyVyyyVAkHkkNUUk4UAALAyLyyYyrcptAAMhhUkjHkmCZchV;vVhNkHUVYlvvLNppjjptjtjj#ZZcckhM4ThhjctcmhcCOjhY;,;;;;;;;;vvvv;;
v;vvvvvvvvvvvvvvvv;;,,vXrArrLryrvytjcckmkkUt0Z0ZqcXUMrM4UccpctccHH4kmNvNbe@v;vvktZ6e9OOD#OO6DcAv,:;;;;,,,,,;;v;;:;,,.,.,..           ......,.,,;;;;;;;;;:..,.......,.,.;vvvvvvvvvvvyVylyyryryryryArX4mkwrrVVyTyrLrVrVVlMm2HhAhwX4mmHkUcGeZXlvVwUUHwyvv;vvrhHkctpjjtbqO#tmcMMhXMUcjtpXycDOZHA, :;;;;;;vvvvv;;
v;vvvvvvvvvvv;vvv;;:;,lU4LryryryYVjccHHHcUkm0pCCqTwhXAHCbctctmtmt4wwUylmDGCvvAvvtj#O699##C@O9#CNv;;;;,,.,,;;;;v;;;v;:.:,:.          ..,,::::;:;;;;;;;;;;;;;;;;;;;;;:;;;;YyVvlvYvYlyyAyrLAMwTUhUhkhTAMrrVYlYvLrArrLLyyVVyUUUMwMhTUUmmcUHCGjUyLyAwkUUryvX4mHMVANmmjctcCO#tjchTNhhhccjckAcbZ@cy; vy;;;;;;vvvv;;
;;;v;;;;;;;v;;;;;;;;;vXbUMyrywALyCqbpC20HHUkHjtqpwMTAAUbtjccmcmcHUMNhAytZGjvvAvVUtj@O6GO0pt2C@9eb4Vv;vvv;;;;;;;;;;;v:..,..   ... ...,,,,,,:::;:;;;:;;;;;;vvvvvvvvlvvvvvvvVvvvYlVYyyrrArMMhAwTwwhMTAwANArVyvYU4rTAAyyyrllvTXUXUTXwHktmkXCZCNHtMvrUUXUwkj#Z0kkhUHHcCckAUjjtchhhXTUmtmck4UbbDqcy;.yTyv;;;;v;vv;
v;v;;;;;;;v;;;;::::;vvkjkrrrTwXrACZC02bjmUHkmmtjtMwATMtctctmcmcmcNhwXMTj99jvlAvYmt2ZGeeOqjmNc2OGgOtYlMXMAyryVlv;;;;;;...,.,...,.:,,,:,,,:,;;;;;;v;v;vvvvvvvvvvvvlvvvlvvvlvYvYVyVyyMrArTMTAwLArArryAAhAAVllvlHNAAhArVLyVvVLUkH4NwU4mccUktOCkH2ylrkXUhXcqD6G9@pmmUt0bXrhpt2chA4NhUptpcHXkcb#OqHv. rHT;;:;;;vl;
v;;v;v;;;;;;;;;;:;,;;lUtXMLMAXwArjbbjjctHmmmHckk4kNk4HctccccctmcHUwXTXkCO@MvVmUrkptCqZZDZCkrAXhHp2m4AXXkUXwwMwrLvlvlvv;v;;;;:;,:,:.,.,.,.,,;::;;;vvvvvvvvvvvvvvvvvvvvllVvVlYVrArrwTXhXTwAXTALALAyVYyVyYvvlvVcckmUXYylylyVLMkU4TXwHkcmH4pD0UmjyvAHmmckUUtCZZD@Cjtt0t4r4mtjtwXNkTUctmcHmmcc20@Uy: ;tcAv;;;:vv;
v:;;;::,,...:,;::,,,vVkUNMNAwTwLMt2jjcjccmcmmkH4kUU4kHjctmcHcmccmwhwwAcC9ty;AcmrHjjHckjD90cMNAAUCtcUMAMrryrLrrNrArTAAvvvvvYvvvv,.,,,;:::;:;;;;;;;;vvvvvvvvvvvvvvvvvvvvvvYlVvlYLyrAXwXhwhhThAArrLLVyYyYlvvv;vjctHcNVlyYLyyllVXwNhUwUkkXmcptcHtVvTttpjmMUmpp00#0bj2jCjthXHbbUhUUU4kUHkmHcHcmC2Hky  V@gjyv;,:vv
,.,. ..        . ,.,vAkUhhMhAMAALjj2tjtbHmHmkH4kUHUk4HHtctmkUHkckhrhyMc0ZtVvrcXrkbctcct6G@mUXhTm2CcXMUhUryvv;vVrMkHH4TrAAwryVrYvvvvvYVVyyyVylVvlvvvvvvvvvlvlvvvlvvvvvllyVrVVyAAArwMMAMAMyrLyVylVvlYylVvvvv;YtcHkUMvlvVyLVyYVAhMhhUUHXUHjcjttHyvNttmjmXUtp@00CCpbt22Cjwrmb@UXUH4k4UXkHmHcmpp2C9b;  ;pqtMv:,;y
.                 .,vMHXhTMMXrArMj0jpjmX4XU4kUkUkUkUkUmmtttUkkHkmMwMMAtmHwyVHtkAHttmjtpDOCtHmhAyVvlvrXkkHhLvv;;;VyrrwTUXUAyyrLylVYVYVlVYVYylYvlvvvlvlvvvlvvvvvYvlvVlyYVVrrMrwMUhwMwMAAArLyyYVVlvlvvvlvvvvvvyUhjbjmUArlVVyVVYTwNT4NHkkUctjcpmmylTq2p2pwAXb@@pqCbpjcjjtyMUC0mNNNkhUUk4HkmkHUH4mc0k;  .cZjrv;;v
                  .;lAMArTLNU4VyrtCptCHNNU4kUHUkUHUk4kkmmjccUHUHUUh4MUtHUMlykbkUjbtttpCCCqjcHhVYvvvvvyAUC9Hrvv;;;;;;;vvvvvlyVrVlvylyllvvvvvYvlvvvlvvvvvvvvvvvvvvvYlyyLyryArTAwMhAhrMAMrArAyrVyvVllvvvvvvvvYTM4COjjkhyVvvvyllrXMNMhMwMhhkmb2tkXYACO0Z0HrwjOqqqCpC20bCtHUmp#2Xy4whUtXT4HkHUkXUXkHUAv. .rcCrv;v
                  .;VLhrrrATcUVvTH@jbbtwUUkUkUkUHUkUkUHmttjmH4HUHXUX4htkHMYycj4XjjpcpjZb#CpcHwMVvvvvvvYAc0mVVYvvv;;;;;;;;;YlVlvvvvvv;;;;vvvvvvvvvvvvvvvvlvvvlvlvvvyLMwNNXhUwArALryAyrrAyLVyLyllvlvlvlvv;vlrrMUOZ2tckkrYvvvlvThXTNTryrrwUcj#cHAly0Z#CDjMLc#Z0@CCb0C@CC2pHHm@MYMTykphVkUmkk4HUUUHNNY;  :Nq#yl;
                   ;YTXTyryNHkVVAtbbtCmUkHHHUHUkUkUkUHUtcjcmHkXkU4h4hUUk44wmq0TUtjmmtC2Cb0ppkTyyvlvlvvyLVyYvvVVvvv;;;vvvvvvlvlvv;;;;;;;;;vvvvvvvvv;vvlvlvVvYlyyryrLAMTAXXHkkTAyryLyLVyyyvlvvvvvvvvvllvvvvYyALUjCpptpjmAyvvvyThMhNXyArNhHc20jkkUUbZCbq6UATcCZ0@CqCqCq#O0tU4ChlNArkjTrUmkHkkkUXHUUTv:   rCOHv;
                   :yHtTVlAXUMLVrk2cjcckckmkmUHUHkH4kUkcpctt2tckHhXwUXU4khkk@pAHCpbjCppjpjbjtUNLyvvv;vrrryv;;;;;;;;;vvvvvvlvlvv;;;v;;;;;;;vvv;v;v;;;vvvvvvlvlVLyyVAM4XUhTAwMryMrryArArwylvVvvvvvvvvvYvvvlVAryVUjZtccbckMyVvvlyX44MAyrrhUcp@pCC0CCC02ZgCryAbqCC0C@0ZqZDDtcUbkXwwAmthLUUkUkkmmcHttkvv;  ;lcbUy
                   ,Yct4vvrkNUwXTUmcHHmmHcmtHHkkUHX4whNj2jcbZ#tHX4XUXUhUXN4mC2AtC0b@2CcpjptjcmNMrVv;:vVAyr;::;,;;v;vvvvvvYYVYVvv;v;v;;;;:;vvvvvYvlvrrMrwVlvvvVyArANkThwwvvvvvlVyVyVVvllVvYllvvvvvvvvvvvvvAwrlVU0bCtjjjUUrVvv;VHjUAyAyLTHmCCqC@CZ00CbqBCMyhj0pCjqCO#Z#DZCmtpjmXLrk0MrXkUUXHmjtctqUvv;  .vUb2U
                   ,yHthVvA4XwwAh4pttcm4kcmrrT4N44UXXNHb0tctObtU4TNhUNX4kkpCD00bb2CCCtjtptpttUMryvv;vvvvlvv;;;;;vvvvvvYlVyyvvllvvvv;vvvvvvvvvvrTALkNHkHHtcjctHUNUXhyyVyVYvvvYlYvVvvvv;vvYv;:;vvvvvv;vvv;vYhMhTmjCtcmtHkTMVv;;ytpHrArrrNXctbCCC0C00#CZG@Mrw0CbjpC#b0b#@Z2jtpccNrymmhAX4kXUUcttHjj4vy;  ,vHHHh
                  ,rkmULvVwUwALALNUUUHMwLAv;vMwXw4kHNrVp00tjj2tcHcXXwXhUUtj#q#C@bbb#pptptjttmtHHVvvvlv;vvvv;;;;vvlvYlylLyryVlyllvvvvlLVyVVyAyMXHkHccmcHcHHmtmMvvvvvvvlvYlyVyVVlyYyYYvlvvv;:;vvvvvvvlvYYrh4XkUHUUXUUkUk4UwAvv;lAUThwhMhTUkjjCbqjqCZZDZOpUArUtC#@@ckkbZZ0022cmkkTMUHXkXkUUUHkcHHHmkHL;vyyAT4Nw
                  v4tkXvvykNMrryrMUhHUkwV:;vhNNwNwNhTlvAkktcttjjp2Hw4hNhkmbqZ200@jb##j2tjcjcccbHYvvvlvvvvvv;v;;vllYYrrAArVLlVlvvvvvvyyArAM4UHHtcjkAvvvvv;,;:;::,:::;;:;;vvvvlvlvVYvv;;v;;;;vLyyVyy4kHUHHHUU4kUUhUhUXkUkUkMyvvvvywXUUUTwMHcppC0qbZ#GZOOCULVrk0ZgCkAXjOZ#0ObkHcHHXUNXNmHHUHkkkHUHUmkl;rH4yLUHr
                  vUkc4VlyMAyrVyVMkkUHUNVvVUUMrXMTLMATyMAXHbj2tpcjHXhXAXH2##0Cb@0qb#@jtbjpctmtjmvvvlvYvvv;;vv;;vvv;vvvvv;;;;,::;;vvrrMrMrNwXN4wMyv;, . . ....;;;:;:;,;:::::;:;;;;v;;:;:;;vvXHcmtmttCtjtjkkkcHUXUXXhUkHNkNMVv;vvAwkXkXhLh4ttCpCtbCq#ZO@UAVyywcZ9#U4Xj#O0ZZChcHjkNXX4kkHHmkmkHkkUUkcV;vYvvvkmr
                  hkUXMVyVrYyllvYrHXk4kh42bNXAAwXArkGqhyhrc2CjpjpcjcmwANjbq0@0#CCpbjbpptCtpctt2HMyVYyYlv;,:,;:;;;;;.     .;;;,;:;;YrMAMAALLVryv;:..,,,;,;,::;;;:;:;:::;:;;;;;,:...,:;;;:YAUcbjjHccttccjcjtbj0CO#OOO0CkkU4ryvvvlYyMUXXAAwHtpjbtjtCj2CGCvvrVVltZ6ZcHcc00@0ObUHccHhXhhh4XmctHkUHUkUcmV,::;;vXmw
                 ;cUyyyyyyVyVylyVMXXXUXkkpjUVTwwThAje9XrMXHbj2jpj2jCtUrkcpb@C#0022jCjbtjtjtptpttUXyVYylv;;..   ..,.,.,.,,;:;:;:;vvvrLyYylv;;;;;:...,,:,::;:;:;:;;;:;:;:;:;:;::,:,,;;;v;;ykctHmHmmtcjmmctcjtpbZZOD9Dq2H4mwAyylYvv;LXUhhAUHppCjjjpjC09Bb,vkUlLtOgOjbcHcbCqq2HmHm4NwXMwrhHjcm4UXUkmtml,.:;vvXXH
                 vtwyVryLVyyAAwAXh4NU44UHHcUVvAwUNkmjctHkTUkttptjtjjtwAUctbqZ0ZC0jbbCjjtptjtpjbckhrYVyrvv;,         :v;;;,..:;lLAArLV;vvl;;;;:;:;:;,:,:,;,;:;:;:;:;::,,,,,:,,,;vvvv;v;;:YMmmttjtjtckmHttjtjt2@DZDDObbkkk4AAVyvlvvLHNUww4cjpjpcmHCZOCt;.yc4MYkO6O2jcUcpCC0tckmUkTXMNkjmHhMh4X4XkmCkv.,:;vvXkm
               :vHkwrryLyrrwMXh4NUXUXUXHHHmUvvLUUkkjcttcwXw4mptjcjcjtcUmmtc2C#C0CCCCCbHmHcmtt0CjhwyyyrLY;;.          ,.,,;;vvrrTATryvvvvvv;;;;;;;;;:,:,,:::;:;:;:;:;,,,,.,.,.:vlvvvv;;;rHwA4AwMTTwyM4cjCjjtpjjtjt2ttmtmkwMylvlvlvyMTMMyhUcc2tchUbGjr, AtMThvrGO#qqtjc2jCb2mmHmUkNXTHjcNAymUHUHmC#r:,,;vvvkNX
              ;wmkhrALLyrrUXUh4wXw4XU4kUkh4MyvhUHUcccmtkUUkkpctcttjjpjpmtmcm2CqbqC00#pHUkUHkCOZmwAwrAyVvv;;,.            vwNkXUwwAV;vvv,.,:,;:;:;;:,:.:,::;:;:;:;::,,.,..,;:;;vvvvv;;;;k#AyryYVlyVrA4m2pbj2pjtptjmcmjcjXAvvvvvvvVlylVVyVrNtjjHk4tjHl,.H0HhUvr99bqqZZDjptppjmcctkkUUXUNXwkmtmcHC#C;.:;;v;YH4y
             .vkNNrTryyryThUw4hXwNhUhNXtUhrAyAmbmtHmmcmkH@CctjcjctjCppctmcmtt#2bC#C@qCHmUm4U2G#cArTwArvv;v;;:,...... .   vrryrrLVY;:,,,,.....,.....,,:,;:::;:;,:,:,,.....;vvvv;v;v;;;;;H@4YyyyVyVryThc20p2p2ttmtmkUtj#CkrvvvvlvYvYVLVryLrUmjmm4wwkr;.YjObtUvyGZ0CZ@OOCcptbccmtccUHUkhXXUUcmcHHbD2v.;vyyVLk4A
             ,HHXTNTMLryrTXw4w4hNh44XwH@@pHTArmtttcUmHmXcDDHcmcctt2pjjjmtctmjjCC@CCC@bjjjjtkCODchyNMwrlv;:,..    .. ;VyVVvv;;......    .,.... .     ;;;;;:;:;::,,,:.;;;;v;;;v;vvv;vvvvvrjXhhhrArMrAwUUccjctmmkmHmUmcCCjNAvvvlvyvYlVlVvVyM4cmHkHVrkl ;mZZpmHvAOO#Z@Z@ZjttppjmtcjHXhkUUUUTUcmHHkZOm;;rXUUhwATy
             LOjMhwNThATA44Uh4hUX4N4w4tGZqpcAXHtjpmHUmHHcqCkkckmcjpbjjtjcjtckHc#qCp2pCbCbCpcc0ZZccUNTrYv;;.          ..,.:;;,. ... ........,       .,;,:,;;;::,,.,.:vvvv;;;;;vvv;vlyVyvyhHmcUUUHUUUHUkXU4kUk4HkmHbqCtHUXrlvvvVrrvVllvvvlVTkkXmkrTk; ;mqZcHUvLOOZ0D#Z0btjtbcHHccmwNNUUH4Xc2jtmcZOA.;ccH4cAvvv
            ;cjUXTNAwMwTwN4h4NUhXh4hwNcjqp@0qttmtcmUUUHmjpCkmmcHjjpj2tjtjtjmkUt0@2CpbpCpCpbkkjOOqtmNXrrv:,.                 .;;;;;;:;;;.. ..        .....,,.,.;;;;;:;;;;;.,:;vvv;vMwhAryMhHU4AXwXNUw4NkkHkHUmctcpCqcUTXMLvvvvyNryyVvlvllVrHUkUHkmv,.YH#ZtmU;rD9#Z#D@OjttjjtkcmcUhMXhkUHhtjbt2H0cV ,yTrMwV;;:
            kpNNw4hhwhwTTXw4TXhXw4wUAUUcCCC0jjctctmHUkUtp2ttcjttcjtjtpcpcjtcHccCp0CC2CpCp2jmkjZOCjmNXUAy;;,.                 .;;;;;;:;:,................ .  .,,;;;;;;;;;,,.;;vrrYyANhhhHUcmmUUUkUUkcUUXNhkUHjZCCbbckTTU4yvvvvlrrlYvvvvvVyNHHUkHtV. lc2j2tHllcObq@#qDZ2cjtpttmtcHhNwNXkUkktmccHh4; :rMrYv;;:,
           y@HXNkXkUUUkAhwhMhMXMwXHUkkkt0p#b2ctctcckHkmcbpjtjjpcjtptttjjjttmtmtmj@@0qjpj2tjccmtC9ZqcmUTrV;;,.                 .;;;;;;,::,.....,,:,,...........,,;:::;:;,,.,;;vALryAAwA42q0#00CCpqCGGObmyyTkk0@#0qjHALAcjprvvYvyylvvvvvvlTUHXUUmcy  LjtcmbVvX9Ctmct@#OttcjttmjcjmXTXNU4kUUN4TU4w4vvlvV,    ..
           rjUhU4kUkUmkAMhATMhMMyHmHHmUjpC2CjtmtctmHUmHjp2tptjtpjjtjtjcpjjttmmmccZqZq0pjt2tpjptZ9OjckHwrv;;:.                  ....,::,:,,.......,.. ... .....           .:;vYyrLyrhMAh@q@bCpCpCjCbjkUArLX4HpZ0Z#CUXVAc@bNvvlyVyvvvvvvvlMcHXAwUA;,:Njjmbprv4#0mmUcCDZjmjtjtttjjthhwXXkUk4kU2ckXkvlT         
          .MpUkUkUHUHHmATMNThMhMMUckmHHt0pq22cccjmcHcmctpjjtpj2tjcjtpcjjjtpcjcjtjC0CCppjpjjtCpjCeZ0ctt4ylvv,.                       ...........                         ,;vvvlyYrrArAAXmjt2jjcjjbpHyyyyHCjbjZ@pmcHH4tmj2pMvvylyylvvvvvvvwckrrr4v:,vhpcHUULrcOtHHcm0ZOjttjcjttcpc4wNwUUmkkXmOq;HqvvA         
         ,vHjckHkHUHkHkXAwwXw4N4hkHmkm4jqqbqpjtjctmcmcmtcjt2ppjpjpttt2tptjjbbbj2tpjpjbjbjCqZbjUkj##OOb4TVv;,.                        . . . ...     . . .   .       ,;vvvvvvv;;;;vYvvVwX4wcj#2cm2bHlVLX4j0@b@ODjkATUm0@cjHNVYlyVyvvvvvvvVAj4rrM4V,,vXmcwrlytOZmhmmtjbjjtjcjtjcjttMTMhwmkmUNUjy vevly         
        .vcctkHUkkHkHkHTwMXh4XUhXUmHmUktO##@0cjctccmtmtctctjCjbjjtpp2cjcjcjpCpbpCjbjpjb2bb##@cNhtbOGejUMyv;..                   .   ..,,,.,.,.,:;:;;;;;;;:;;;::,:,:;v;v;:.    ..:;vvyrmmHcpC9ZqjkY:vL4jjbjb2OZpHXr4m02tHmNwllVVYvvvvv;vvTcmrVrNv,;YMkUXrYyCDDUhHjtpttt2tpcjcjtpchMwAUmckHTUH;  jrAy         
        vUttkHkHUHUHkHkXMhwXh4hXhkHHUkXpODq@ptcjttccHcctcjcpbbpCtptbtjctcttbjCjCp2tjtpjbjCC#CjU4HbZ9ODbkVv;;..                 :,,,;;vvvvv;v;v;;:;;;;;;;;;;;;;,:,,,,.,..      .,,;vLyrAm4tp0GBCkvvvr4tj2jpjCbtmcHmmjbCctHXrVvyVyvvvvvvvVTjHyvANv,;vThUNhyMt#0c4ctpjptjcjcjcjtpjcThMNwmkHXUjV     ,          
       ,UccUkUmUkUHUHUkhwMww4hUhUNUUUhmbCpCtmHcmtctcttjtppCbq0@bCjjtjcjctcjt2jbtjctcjctmtmtcjctHttqZGZ0Hrvv:,.                  . .,;;;;;;;:;::,,,,.. . .       .....       ,;vvv;;vvvvvHjC#eCY:;lHUpq#bbpCptHmcpp2tbpbjchXyYVyVYvvvvvYy4mHvvAkv,;YAUhUwUXccpcccjjjtjt2jttjctcjUXNUNUHmUhk2.                
       .MckkUHkHUHUkXkhNMhrhw4N4UkXkXNmqj2jjHmmcctctj2j2jCb0p0CCb0jjjjttcjtpjbjjctcjcjctccccctcccjbDOObkYv;:.. . .                                             ..,..       ..;;;;;;;;;,vcDqCjA:;YmjC#66Ojpj2ctHmcpj2jpjpckNAlYlVvlvlvvVLNcUlvTUv.;vAhUhNNUktmcmtctccmjjCjjcccckHUmHHUmHkUbA                 
       .wkkUkkHkUUH4XNXMhMhMNX4hU4U4Uwmp2tptbj2pjtjtbpbjCpCb2jjjbjbcmHtctcttbj2tjttccmtmtccctcjttcCZ9OZHrvv,. .                                           ..,.. ...           . . ..,. ;pZZHr;vyc0OqZZO#qtmHcmmmtjbjptpjc4UrlvVYVYVlYvMXUXAllAUv,;lT4XUwhhtctmttjctctt2pbttmtcmmcmcHHUHXHN,                 
      vrUUHHkmkmHHkUAArNwhAhwUXUNUUk4UHptjt2p02Ctjj2jb2Cpbjptjmjt02pUUHtmccjjpjC2CtmUmctctcjjbp2cmmC#OZ@H4v;,. . .                   . . .   . ..,,:,... ...         .                 vmCkNMrrMmZ#@0ZqZZ2HU4mkmc2bbjjmH4UUMvyYyVLVylLktUAyMAhwL;vAk4kX4NHcjttt2jttjcC0OO9ZZDZpjctmckkTkN.                  
     vUNUXckkkmUmmcUMyAMXMhT4hXhU4kUUUcjCjpjC2CjptjtppCCCjjtpmccpC@tHUtcckccptjb#@pUUHjttcttCCqjcUUc@DOCbmwvv,. ...   . ,,;;;:;;vvvvVllvlvvvYvv;v;v;:.,....                           .vmckwHmtmtC#0#b0@Ojc4kkckc20CqjHXkUcMVVyVVlYYvVHmkrAMhwNy;vAHH4kkmHUkHUhvwctXTUmtO9g9GGqmjtjjCcUV;                   
    ;UALhUkkXkUHkmmUrLyMThMNwXXU4HkmkkHCCCjpcjcccjctctcjjbcmUUTwAUXNM4mtmmcjjjc2C#cUTcttcjcjbC2bHkHj0O#CttUrVv.. ...          .,:;,;;;;;;;;;:;.. . ... . . . .                         vHHXXj@bCb0CZZ#00cjccHmHHHqq0CCkk4HcXvVvvvrLryhkmUwh4wXAr;vyUHckmccyyrrlvrqm; ..;vrwwMUwTLrAMAY.                     
   .mkrA4hUXkUkUHkkhrVrAMMNhXwUN4UHHHUcb0bpttmcHccb0Z0@0bttk4NAvYlLrTwUkmkcHpjpt0bt44kcmtmctpjptcHjmjO90pccwrv;..               . . .....               ..:::,                         vmmtc2CCb00qqD@DpmcjttctHtDD0#cUXkXrvvvvyUkmmmkkXUUHHkTryvvrwHtmwyv;.;vwHqbY     ,..                                 
   vHUVNNUwX4kXUXk4UwArAMNwXwUXXwUkHUkHCCCtptjctcj0G9G99CppcXHXMVyYATNAXXkUUmtcjjCck4kkmmHmCtjcpcpcccOOZtj2mrl;:.                . ....,                 ..,..                       ,vrwkcCCCpCC@0Z#ZZCHtcttbpCCGOCkXMTryv;vVrkmmUkUkUk4kkmUNrrvvLwUHXA;,,,;yk#m;     .                                    
  .rcMywUNXhUh4hNUU4kkULwMhwNw4NXhUUctbbCpbjCbCjCj@Z9O9D@jpttttjjXv;LUUMUkkUtttcq0jX4kkkmUtp2tptjctmtZOqbjbXAv;,,               . ....,,.                                           ;rZHrXct0bbjCC@@D#OCjccmjpbpq0qUryrvlvYYrUmUHXkNUUkUHUmkkhMylvrTHyv;,.,;LXUv.   .,.                                     
  VXNrVkXUhXNUhNAwhkkmUTMNTXX4NUNNw4HjpC202pccmmc@0##Z0#qqjbctcjccNyYwHk4HUkHtmcjjcmkHUHkHmbtptjcjctc0@qb0pkLv;:,.             ... ..,,:.                                         ;cj##tUct2bCpCp0C@b@CbpCjbj2jCjDCcl;: .vy4Up0qtjtjctHcmcHH4hyLVyVyYlvVyTrAyv      :,                                      
  NXM;YkUNUUHNhAhUqmrTtMhwNh4hUhNMThmjbt2tpUwryytODq#jjjCjptjHcHctCHrvccmUUhkkkUmkcHmUk4kUctjcjcttjctpqbCb0tHTL;;,.             . . ,,,,.   .                     .             ;H99ObCtpjbpCjCbb2Cjbj0C0C#CCpC20Oebv   .vUXmt#CjcjtpjUUHUkkkTAyyVyVAMHtCt4v                                                
 ;cmA;;HTNh4v;vHqG62;yqNMUw4TUN4wHcjp0pcXkmHTyvYXGZZ0pj2jpttttHcHjbbmAvwkkXUUk4kUHHmkHUUUHmpjptjcjtjtCCqCq0q2mT;,,                  ,:.:.                                     vjGe#Cjpjpjtj2pb202bpCjbbCCq0qjbjjt@bk:    rcHHjjjcjtjjpAyyrAtcmUNrTrhM;.,.:                                                  
,LtmUVVNNAAw;.yC6GgCvvqUNN4w4h4wNj###CpmkHckkylvh0qb02C2@2bttmmHttCjmAvykXkUHUkUHmcmckmUHkHtjcjcbp2cpp0q@CCpCj2;,..                 :,,,: ...             . .     ...       ,YGe9C2jpjjtjcjtppqpCC0C0CCC0Cq2CC#tXHUVv   ;MtwhmC0CjpcbjXv;;vUwYl;;:;;,                                                       
vmcyvvrUw4hk;.hOctOZvlOcy4w4hNhwMCCq0bctkkkmk4VvV0@@pbjqC#2pcmXkmtjjUAvAXHUkUkUk4cttHtkkkHNUXUwUmbt2jCC0C2cccCOr::.                .::,:,:.,,:.  ...   .     .   .       ;ytB8OZC0jpjbj2tptptpCC0qC0C@bCbbpbj#ZbTwLMLl.YH2kVLcCGqjttj2Mv:;lVv;                                                              
ymc: ;XwhrNNv,mCrYOj;rGUyh4Mhw4TNCq0qpjcUNkUmwyvLCZCCjCCqC2jCckryTcbbNvvUUkUUUkUkmjctHHhHXHUyvlLwM4c2Cqb0bqjjcChv;:.                ::;,:,..,,.   .           .       :wC#99Z0@C0bCtjjbt2tjttjqb#C@C0C0bCpbjp#92NrMrv;;teDwvrUCOqHmHj2mv:;;:;                                                               
LcM  vHkTwTTyrXHvV#j;h#hVXTNMUHjjZ0@0C22mmHjmNylY0DZpjt0b02ZODtUyAHq2UvVXkUHUUXkkcHHmmHmkkk4vvYhMXNtC#0Cb#DOpmcHLv;:                ,:,:,,.,,:.              .      ;AUZZOOO@#bCpCbpjbj2tptpjCb0C@C0b0b0b0CbjDOm:;ry:;y#Gj;;AThkAhNkcObv                                                                    
kDN  YbkXhXh4Ukv:ltp4ptrAwNANhHmb@Z@#CCbptjjtwrvvwOODctpbCC@B9ZcUrHCbXAr4Mh4kU44HkHHHc0jjcjkV;VwwXUk@DqCC0DO0mmctrv;.   .           ,...,,;,;;,   .           .    vpGBOZC@0#CqbCpCtpjbpCpCpCj@Cq0qC#CqC0CqpbZG4..yY;lC0Cy  yArvvrUkCCm;                                                                    
peX  y0hMqjHrwc: ,AZCG2XMXMTM4Ujj#q#0CpbjptjmkAV;ycGGO2bj0CqD9CjkrrNMTU4wATkUHXHkckmUpCCjCC0mA;;;vvAXCjZ@qCDbpHjpTv;.  ,.           .,.,.::;:;.  .               vrmpOOD0qCqbb2bb0pbtbjb20bCpC0@Cq00Cq0qCqCCp96m: vVvXgGm,  ;vv;;wkqDqA,                                                                    
.XD. ;pVrcjXYrGv  4920kyrrAyjpptjtpttmcctctccUXYv;M#9D@C@0@CZDCHHyVYyYryArNhUTwNHUkUUXccptbbq4v::;vrhhc#9Cjc2jjctAv,.                   ......                  vCBg9qZ#0bqC2pCjbpCt2tpjpjbjbjCp0b0C0C0C0Cq2ZCM.  ,rCmGOl        ;ykqw;                                                                     
*/


// Dream on, dream on, dream on until your dream come true... /Aerosmith - Dream On/
