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

// During compilation use one of these, the granularity of the padded 'memcpy', 4x2x8/2x2x16/1x2x32/1x1x64 respectively as GP/XMM/YMM/ZMM, the maximum literal length reduced from 127 to 63:
//#define _N_GP
//#define _N_XMM
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

// Dream on, dream on, dream on until your dream come true... /Aerosmith - Dream On/
