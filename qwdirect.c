#include "include.h"

int test_empty(unsigned char* srcbuf)
{
    for(int i = 0; i < pagesize; i++)
        if(srcbuf[i] != 0xff)
            return 0;
    return 1;
}

//**********************************************************
//*   Настройка чипсета на линуксовый формат раздела флешки
//**********************************************************
void set_linux_format() {
  
unsigned int sparnum, cfgecctemp;

if (nand_ecc_cfg != 0xffff) cfgecctemp=mempeek(nand_ecc_cfg);
else cfgecctemp=0;
sparnum = 6-((((cfgecctemp>>4)&3)?(((cfgecctemp>>4)&3)+1)*4:4)>>1);
// Для ECC- R-S
if (! (is_chipset("MDM9x25") || is_chipset("MDM9x3x") || is_chipset("MDM9x4x"))) set_blocksize(516,1,10); // data - 516, spare - 1 байт, ecc - 10
// Для ECC - BCH
else {
	set_udsize(516); // data - 516, spare - 2 или 4 байта
	set_sparesize(sparnum);
}
}  
  

//*******************************************
//@@@@@@@@@@@@ Головная программа
void main(int argc, char* argv[]) {
  

			     
unsigned char datacmd[1024]; // секторный буфер
			     
unsigned char srcbuf[8192]; // буфер страницы 
unsigned char membuf[1024]; // буфер верификации
unsigned char databuf[8192], oobuf[8192]; // буферы сектора данных и ООВ
unsigned int fsize;
FILE* in;
int vflag=0;
int sflag=0;
int cflag=0;
unsigned int flen=0;
#ifndef WIN32
char devname[]="/dev/ttyUSB0";
#else
char devname[20]="";
#endif
unsigned int cfg0bak,cfg1bak,cfgeccbak,cfgecctemp;
unsigned int i,opt;
unsigned int block,page,sector;
unsigned int startblock=0;
unsigned int stopblock = -1;
unsigned int bsize;
unsigned int fileoffset=0;
int badflag;
int uxflag=0, ucflag=0, usflag=0, umflag=0, ubflag=0;
int wmode=0; // режим записи
int readlen;

#define w_standart 0
#define w_linux    1
#define w_yaffs    2
#define w_image    3
#define w_linout   4

while ((opt = getopt(argc, argv, "hp:k:b:a:f:vc:z:l:o:u:s")) != -1) {
  switch (opt) {
   case 'h': 
    qprintf("\n  Утилита предназначена для записи сырого образа flash через регистры контроллера\n\
Допустимы следующие ключи:\n\n\
-p <tty>  - указывает имя устройства последовательного порта для общения с загрузчиком\n\
-k #      - код чипсета (-kl - получить список кодов)\n\
-b #      - начальный номер блока для записи \n\
-a #      - номер блока на флешке до котороко можно записывать (по умолчанию до конца флешки)\n\
-f <x>    - выбор формата записи:\n\
        -fs (по умолчанию) - запись только секторов данных\n\
        -fl - запись только секторов данных в линуксовом формате\n\
        -fy - запись yaffs2-образов\n\
	-fi - запись сырого образа данные+OOB, как есть, без пересчета ЕСС\n\
	-fo - на входе - только данные, на флешке - линуксовый формат\n");
qprintf("\
-s        - пропускать страницы, содержащие только байты 0xff (необходимо при записи ubi оразов)\n\
-z #      - размер OOB на одну страницу, в байтах (перекрывает автоопределенный размер)\n\
-l #      - число записываемых блоков, по умолчанию - до конца входного файла\n\
-o #      - смещение в блоках в исходном файле до начала записываемого участка\n\
-ux       - отключить аппаратный контроль дефектных блоков\n\
-us       - игнорировать признаки дефектных блоков, отмеченные во входном файле\n\
-uc       - симулировать дефектные блоки входного файла\n\
-um       - проверять соответствие дефектных блоков файла и флешки\n\
-ub       - не проверять дефектность блоков флешки перед записью (ОПАСНО!)\n\
-v        - проверка записанных данных после записи\n\
-c n      - только стереть n блоков, начиная от начального.\n\
\n");
    return;
    
   case 'k':
    define_chipset(optarg);
    break;

   case 'p':
    strcpy(devname,optarg);
    break;
    
   case 'c':
     sscanf(optarg,"%x",&cflag);
     if (cflag == 0) {
       qprintf("\n Неправильно указан аргумент ключа -с");
       return;
     }  
     break;
     
   case 'b':
     sscanf(optarg,"%x",&startblock);
     break;

   case 'a':
     sscanf(optarg,"%x",&stopblock);
     break;
     
   case 'z':
     sscanf(optarg,"%u",&oobsize);
     break;
     
   case 'l':
     sscanf(optarg,"%x",&flen);
     break;
     
   case 'o':
     sscanf(optarg,"%x",&fileoffset);
     break;
     
   case 's':
     sflag=1;
     break;
     
   case 'v':
     vflag=1;
     break;
     
   case 'f':
     switch(*optarg) {
       case 's':
        wmode=w_standart;
	break;
	
       case 'l':
        wmode=w_linux;
	break;
	
       case 'y':
        wmode=w_yaffs;
	break;
	
       case 'i':
        wmode=w_image;
	break;

       case 'o':
        wmode=w_linout;
	break;
	
       default:
	qprintf("\n Неправильное значение ключа -f\n");
	return;
     }
     break;
     
   case 'u':  
     switch (*optarg) {
       case 'x':
         uxflag=1;
	 break;
	 
       case 's':
         usflag=1;
	 break;
	 
       case 'c':
         ucflag=1;
	 break;
	 
       case 'm':
         umflag=1;
	 break;
	 
       case 'b':
         ubflag=1;
	 break;
	 
       default:
	qprintf("\n Неправильное значение ключа -u\n");
	return;
     }
     break;
     
   case '?':
   case ':':  
     return;
  }
}  

if (uxflag+usflag+ucflag+umflag > 1) {
  qprintf("\n Ключи -ux, -us, -uc, -um несовместимы между собой\n");
  return;
}

if (uxflag+ubflag+umflag > 1) {
  qprintf("\n Ключи -ux, -ub, -um несовместимы между собой\n");
  return;
}

if (uxflag+ubflag > 1) {
  qprintf("\n Ключи -ux и -ub несовместимы между собой\n");
  return;
}

if (uxflag && (wmode != w_image)) {
  qprintf("\n Ключ -ux допустим только в режиме -fi\n");
  return;
}

#ifdef WIN32
if (*devname == '\0')
{
   qprintf("\n - Последовательный порт не задан\n"); 
   return; 
}
#endif

if (!open_port(devname))  {
#ifndef WIN32
   qprintf("\n - Последовательный порт %s не открывается\n", devname); 
#else
   qprintf("\n - Последовательный порт COM%s не открывается\n", devname); 
#endif
   return; 
}

if (!cflag) { 
 in=fopen(argv[optind],"rb");
 if (in == 0) {
   qprintf("\nОшибка открытия входного файла\n");
   return;
 }
 
}
else if (optind < argc) {// в режиме стирания входной файл не нужен
  qprintf("\n С ключом -с недопустим входной файл\n");
  return;
}

if(stopblock <= startblock) {
  qprintf("\n Значение ключа -b должен быть меньше значения ключа -a\n");
  return;
}

hello(0);

if(stopblock == -1)
    stopblock = maxblock;

if(maxblock < startblock) {
  qprintf("\n Ошибка, значение ключа -b больше размера flash памяти\n");
  return;
}

if(maxblock < stopblock) {
  qprintf("\n Ошибка, значение ключа -a больше размера flash памяти\n");
  return;
}

if ((wmode == w_standart)||(wmode == w_linux)) oobsize=0; // для входных файлов без OOB
oobsize/=spp;   // теперь oobsize - это размер OOB на один сектор

// Сброс контроллера nand
nand_reset();

// Сохранение значений реистров контроллера
cfg0bak=mempeek(nand_cfg0);
cfg1bak=mempeek(nand_cfg1);
cfgeccbak=mempeek(nand_ecc_cfg);

//-------------------------------------------
// режим стирания
//-------------------------------------------
if (cflag) {
  if ((startblock+cflag) > stopblock) cflag=stopblock-startblock;
  qprintf("\n");
  for (block=startblock;block<(startblock+cflag);block++) {
    qprintf("\r Стирание блока %03x",block); 
    if (!ubflag) 
      if (check_block(block)) {
	qprintf(" - badblock, стирание запрещено\n");
	continue; 
      }	
    block_erase(block);
  }  
  qprintf("\n");
  return;
}

//ECC on-off
if (wmode != w_image) {
  mempoke(nand_ecc_cfg,mempeek(nand_ecc_cfg)&0xfffffffe); 
  mempoke(nand_cfg1,mempeek(nand_cfg1)&0xfffffffe); 
}
else {
  mempoke(nand_ecc_cfg,mempeek(nand_ecc_cfg)|1); 
  mempoke(nand_cfg1,mempeek(nand_cfg1)|1); 
}
  
// Определяем размер файла
if (wmode == w_linout) bsize=pagesize*ppb; // для этого режима файл не содержит данных OOB, но требуется запись в OOB
else bsize=(pagesize+oobsize*spp)*ppb;  // размер в байтах полного блока флешки, с учетом ООВ
fileoffset*=bsize; // переводим смещение из блоков в байты
fseek(in,0,SEEK_END);
i=ftell(in);
if (i<=fileoffset) {
  qprintf("\n Смещение %i выходит за границу файла\n",fileoffset/bsize);
  return;
}
i-=fileoffset; // отрезаем от длины файла размер пропускаемого участка
fseek(in,fileoffset,SEEK_SET); // встаем на начало записываемого участка
fsize=i/bsize; // размер в блоках
if ((i%bsize) != 0) fsize++; // округляем вверх до границы блока

if (flen == 0) flen=fsize;
else if (flen>fsize) {
  qprintf("\n Указанная длина %u превосходит размер файла %u\n",flen,fsize);
  return;
} 
  
qprintf("\n Запись из файла %s, стартовый блок %03x, размер %03x\n Режим записи: ",argv[optind],startblock,flen);


switch (wmode) {
  case w_standart:
    qprintf("только данные, стандартный формат\n");
    break;
    
  case w_linux: 
    qprintf("только данные, линуксовый формат на входе\n");
    break;
    
  case w_image: 
    qprintf("сырой образ без расчета ЕСС\n");
    qprintf(" Формат данных: %u+%u\n",sectorsize,oobsize); 
    break;
    
  case w_yaffs: 
    qprintf("образ yaffs2\n");
    set_linux_format();
    break;

  case w_linout: 
    qprintf("линуксовый формат на флешке\n");
    set_linux_format();
    break;
}   
    
port_timeout(1000);

// цикл по блокам
if ((startblock+flen) > stopblock) flen=stopblock-startblock;
for(block=startblock;block<(startblock+flen);block++) {
  // проверяем, если надо, дефектность блока
  badflag=0;
  if (!uxflag && !ubflag)  badflag=check_block(block);
  // целевой блок - дефектный
  if (badflag) {
//    qprintf("\n %x - badflag\n",block);
    // пропускаем дефектный блок и идем дальше
    if (!ubflag && !(umflag || ucflag)) {
      qprintf("\r Блок %x дефектный - пропускаем\n",block);
      if(startblock+flen == stopblock)
          qprintf("\n Внимание: последний блок файла не будет записан, выход за границу области записи\n\n");
      else
          flen++;   // сдвигаем границу завершения вводного файла - блок мы пропустили, данные раздвигаются
      continue;
    }
  }
  // стираем блок
  if (!badflag || ubflag) {
    block_erase(block);
  }

  bch_reset();

  // цикл по страницам
  for(page=0;page<ppb;page++) {

    memset(oobuf,0xff,sizeof(oobuf));
    memset(srcbuf,0xff,pagesize); // заполняем буфер FF для чтения неполных страниц
    // читаем весь дамп страницы
    if (wmode == w_linout) readlen=fread(srcbuf,1,pagesize,in);
    else readlen=fread(srcbuf,1,pagesize+(spp*oobsize),in);
    if (readlen == 0) goto endpage;  // 0 - все данные из файла прочитаны

    if(sflag && test_empty(srcbuf))
    {
        //qprintf("\n Пропущена пустая страница: блок %x, страница %x\n",block,page);
        continue;
    }

    // srcbuf прочитан - проверяем, не бедблок ли там
    if (test_badpattern(srcbuf)) {
        // там действительно бедблок
        if (usflag)
        {
            // если сказано писать как есть, пишим как есть
            if(page == 0)
                qprintf("\r Блок %x - признак бэдблока записан \"как есть\", продолжаем запись\n",block);
        }
        else if (!badflag)
        {
            // входной блок не соответствует бэдблоку на флешке, либо нам всё равно что на флешке
            if(ucflag)
            {
                if(page == 0)
                {
                    // создание бедблока
                    // todo: откат в случаи ошибки
                    mark_bad(block);
                    qprintf("\r Блок %x отмечен как дефектный в соответствии с входным файлом!\n",block);
                }
                continue;
            }
            else if (umflag)
            {
                qprintf("\r Блок %x: на flash дефект не обнаружен, завершаем работу!\n",block);
                return;
            }
            else
            {
	        if (page == 0)
                    qprintf("\r Обнаружен признак дефектного блока во входном дампе - пропускаем\n");
	        continue;  // пропускаем этот блок, постранично
            }
        }
        else
        {
            // если нам не плевать на бэдблоки флешки и бэдблок действительно есть
            if (page == 0)
                qprintf("\r Блок %x - дефекты соответствуют, продолжаем запись\n",block);
	    continue;  // пропускаем этот блок, постранично
        }
    }

    if (badflag)
    {
	qprintf("\r Блок %x: на flash обнаружен неожиданный дефект, завершаем работу!\n",block);
	return;
    }

    // разбираем дамп по буферам
    switch (wmode) {
      case w_standart:
      case w_linux:
      case w_image:
      // для всех режимов, кроме yaffs и linout - формат входного файла 512+obb
      for (i=0;i<spp;i++) {
		memcpy(databuf+sectorsize*i,srcbuf+(sectorsize+oobsize)*i,sectorsize);
		if (oobsize != 0) memcpy(oobuf+oobsize*i,srcbuf+(sectorsize+oobsize)*i+sectorsize,oobsize);
     }  
	break;
	 
      case w_yaffs:
      // для режима yaffs - формат входного файла pagesize+obb 
		memcpy(databuf,srcbuf,sectorsize*spp);
		memcpy(oobuf,srcbuf+sectorsize*spp,oobsize*spp);
      break;

      case w_linout:
      // для этого режима - во входном файле только данные с размером pagesize 
		memcpy(databuf,srcbuf,pagesize);
	break;
    }
    
    // устанавливаем адрес флешки
    qprintf("\r Блок: %04x   Страница: %02x",block,page); fflush(stdout);
    setaddr(block,page);

    // устанавливаем код команды записи
    switch (wmode) {
	case w_standart:
	mempoke(nand_cmd,0x36); // page program - пишем только тело блока
    break;

	case w_linux:
	case w_yaffs:
	case w_linout:
        mempoke(nand_cmd,0x39); // запись data+spare, ECC вычисляется контроллером
    break;
	 
	case w_image:
        mempoke(nand_cmd,0x39); // запись data+spare+ecc, все данные из буфера идут прямо на флешку
    break;
    }

    // цикл по секторам
    for(sector=0;sector<spp;sector++) {
      memset(datacmd,0xff,sectorsize+64); // заполняем секторный буфер FF - значения по умолчанию
      
      // заполняем секторный буфер данными
      switch (wmode) {
        case w_linux:
	// линуксовый (китайский извратный) вариант раскладки данных, запись без OOB
          if (sector < (spp-1))  
	 //первые n секторов
             memcpy(datacmd,databuf+sector*(sectorsize+4),sectorsize+4); 
          else 
	 // последний сектор
             memcpy(datacmd,databuf+(spp-1)*(sectorsize+4),sectorsize-4*(spp-1)); // данные последнего сектора - укорачиваем
	  break;
	  
        case w_standart:
	 // стандартный формат - только сектора по 512 байт, без ООВ
          memcpy(datacmd,databuf+sector*sectorsize,sectorsize); 
	  break;
	  
	case w_image:
	 // сырой образ - data+oob, ECC не вычисляется
          memcpy(datacmd,databuf+sector*sectorsize,sectorsize);       // data
          memcpy(datacmd+sectorsize,oobuf+sector*oobsize,oobsize);    // oob
	  break;

	case w_yaffs:
	 // образ yaffs - записываем только данные 516-байтными блоками 
	 //  и yaffs-тег в конце последнего блока
	 // входной файл имеет формат page+oob, но при этом тег лежит с позиции 0 OOB 
          if (sector < (spp-1))  
	 //первые n секторов
             memcpy(datacmd,databuf+sector*(sectorsize+4),sectorsize+4); 
          else  {
	 // последний сектор
             memcpy(datacmd,databuf+(spp-1)*(sectorsize+4),sectorsize-4*(spp-1)); // данные последнего сектора - укорачиваем
             memcpy(datacmd+sectorsize-4*(spp-1),oobuf,16 );    // тег yaffs присоединяем к нему
		  }
	  break;

	case w_linout:
	 // записываем только данные 516-байтными блоками 
          if (sector < (spp-1))  
	 //первые n секторов
             memcpy(datacmd,databuf+sector*(sectorsize+4),sectorsize+4); 
          else  {
	 // последний сектор
             memcpy(datacmd,databuf+(spp-1)*(sectorsize+4),sectorsize-4*(spp-1)); // данные последнего сектора - укорачиваем
		  }
	  break;

      }
      // пересылаем сектор в секторный буфер
	  if (!memwrite(sector_buf,datacmd,sectorsize+oobsize)) {
		qprintf("\n Ошибка передачи секторного буфера\n");
		return;
      }	
      // если надо, отключаем контроль бедблоков
      if (uxflag) hardware_bad_off();
      // выполняем команду записи и ждем ее завершения
      mempoke(nand_exec,0x1);
      nandwait(); 
      // включаем обратно контроль бедблоков
      if (uxflag) hardware_bad_on();
     }  // конец цикла записи по секторам
     if (!vflag) continue;   // верификация не требуется
    // верификация данных
     qprintf("\r");
     setaddr(block,page);
     mempoke(nand_cmd,0x34); // чтение data+ecc+spare
     
     // цикл верификации по секторам
     for(sector=0;sector<spp;sector++) {
      // читаем очередной сектор 
      mempoke(nand_exec,0x1);
      nandwait();
      
      // читаем секторный буфер
      memread(membuf,sector_buf,sectorsize+oobsize);
      switch (wmode) {
        case w_linux:
 	// верификация в линуксовом формате
	  if (sector != (spp-1)) {
	    // все сектора кроме последнего
	    for (i=0;i<sectorsize+4;i++) 
	      if (membuf[i] != databuf[sector*(sectorsize+4)+i])
                 qprintf("! block: %04x  page:%02x  sector:%u  byte: %03x  %02x != %02x\n",
			block,page,sector,i,membuf[i],databuf[sector*(sectorsize+4)+i]); 
	  }  
	  else {
	      // последний сектор
	    for (i=0;i<sectorsize-4*(spp-1);i++) 
	      if (membuf[i] != databuf[(spp-1)*(sectorsize+4)+i])
                 qprintf("! block: %04x  page:%02x  sector:%u  byte: %03x  %02x != %02x\n",
			block,page,sector,i,membuf[i],databuf[(spp-1)*(sectorsize+4)+i]); 
	  }    
	  break; 
	  
		 case w_standart:
	     case w_image:  
         case w_yaffs:  
	     case w_linout: // пока не работает! 
          // верификация в стандартном формате
	  for (i=0;i<sectorsize;i++) 
	      if (membuf[i] != databuf[sector*sectorsize+i])
                 qprintf("! block: %04x  page:%02x  sector:%u  byte: %03x  %02x != %02x\n",
			block,page,sector,i,membuf[i],databuf[sector*sectorsize+i]); 
	  break;   
      }  // switch(wmode)
    }  // конец секторного цикла верификации
  }  // конец цикла по страницам 
} // конец цикла по блокам  
endpage:  
mempoke(nand_cfg0,cfg0bak);
mempoke(nand_cfg1,cfg1bak);
mempoke(nand_ecc_cfg,cfgeccbak);
qprintf("\n");
}
