//////////////////////////////////////////////////////////////////
// Ramsoft MakeTZX 1.02.1
//
// Bugfixed.
//
// Last update: 05-04-1998 23:30
//
// READ DISCLAIM.TXT FOR IMPORTANT INFO
//
// This source works with UN*X compiler.
// MakeTZX is freeware.
// The source code is copyright (C) 1998 Stefano Donati, Ramsoft
// and CANNOT BE MODIFIED in any form without the permission
// of the author.
// You CANNOT compile and/or distribute a modified version of
// this program.
// If you want to port MakeTZX to other platforms, please inform
// us.
// If you use some of this code in your program, you must credit
// us in it.
/////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

int ARGC;
char **ARGV;

char version[16]="1.02.1 for Linux";

////////////////////////////////////////////////////////////////////////////
// The following code has been added for compatibility.
// Additional code by Luca.

int itoa (int n, char *buff, int base)
{ int i,l;   // this only works with base <= 10, but who cares...
  l=sizeof(buff);
  for(i=0;i<l;i++)
     { buff[l-1-i]='0'+n%base;
       n/=base;
     }
  return 0;
}

char *strupr(char *s)
{ int i,l;
  l=strlen(s);
  for(i=0;i<l;i++) if((s[i]>='a')&&(s[i]<='z')) s[i]-=32;
  return s;
}

char *strlwr(char *s)
{ int i,l;
  l=strlen(s);
  for(i=0;i<l;i++) if((s[i]>='A')&&(s[i]<='Z')) s[i]+=32;
  return s;
}

char *strnset(char *s, int ch, int n)
{ int i;
  if(n>strlen(s)) n=strlen(s);
  for(i=0;i<n;i++) s[i]=ch;
  return s;
}

char *strrev(char *s)
{ int i;
  char c;
  for(i=0;i<(strlen(s)+1)/2;i++)
     { c=s[i];
       s[i]=s[strlen(s)-1-i];
       s[strlen(s)-1-i]=c;
     }
  return s;
}

int eof(int h)
{ long l,m;
  l=lseek(h,0,SEEK_CUR);
  m=lseek(h,0,SEEK_END);
  lseek(h,l,SEEK_SET);
  return (l==m);
}


/////////////////////////////////////////////////////////////////////////////

typedef unsigned char byte;
typedef unsigned int word;
typedef unsigned long dword;

typedef struct {
		 byte handle;
		 byte name[255];
		 word bufpoint;
		 dword length;
		 dword xferred;
		 byte buffer[0x4000];
	       } fdata;

typedef struct {
		 double freq;
		 double rfreq;
		 double samples;
		 word tstates;
	       } wdata;

typedef struct {
		 byte busy;
		 byte savedir;
		 byte savebtype;
		 byte savelast;
		 dword saverev;
		 dword savepos;
		 dword savetotal;
		 dword saveprocess;
		 dword saveblocklen;
	       } stackitem;

byte normal=0,         // are we saving a normal block?
     alk=0,            // are we saving an Alkatraz protected program?
     soft=0,           // are we saving an Softlock protected program?
     bleep=0,          // are we saving a Bleepload protected program?
     slocktype=0,      // which type of Speedlock loader are we saving?
     zeta=0,
     raw=0,
     hex=0,
     hifi=1,
     loggen=0,
     spdchk=0;

byte PilotLinkingOK;   // has pilot linked OK?
byte ClickingPilotOK;
byte SyncLoadOK;       // was the sync right?
byte DataLoadOK;       // was the data loaded right?
byte EndBlock;
byte FindingPause=0;
byte ClickSearch=0;
byte SpeedEndBlock;
byte CheckingSpeed=0;

byte blocktype;
dword blocklength=0;
dword processed=0;
dword totalread=0;
byte compression=0;
double silence=0;
dword filepos=0;

fdata inpfile;
fdata outfile;
fdata tempfile;

stackitem stack1,stack2,stack3,stack4,stack5;

FILE *logfile;

/*
byte savedir;
dword saverev;
byte savebtype;
byte savelast;
dword savepos;
dword savetotal;
dword saveprocess;
dword saveblocklen;
*/

//struct date curdate;
//struct time curtime;

float samprate=0;

wdata pilot;
wdata sync;
wdata bit0;
wdata bit1;

//byte tapespeedcheck=0;
int diff=0;
double correction=0;
byte direction=1;
byte tonesearch=0;
byte tonefreq=0;
byte lastsample;
byte flag;
byte chksum;
dword lastpulse;
dword currpulse;
dword wmax,wmin;
word wmaxcount,wmincount;
byte lastpeak=128;
dword revsamples=0;
word nxtloadpos=0;
word expectedlength=0;
dword probend=0;
byte rampage;
dword pilotlength;
double mspause=0;
word msbeep;
dword beeplength;
word blocks=0;
word blockskip=0;
byte checksum;
double ecf=1;

byte add256=0;
byte opcode=0;

byte bleeploader[512];
byte bleepblock[288];
byte bleepgroup;
byte searching=0;

word alkloadpos=0;
//word alkflag;
//byte alkseed=0;
//byte alkinc1=0;
//byte alkinc2=0;
//byte alkcheck=0;
//word alksumpos=0;

byte xorbyte=0xc1;
byte addbyte=0x11;
word slockloadaddress=0;
word loadertableadd=0;
word tableoffset=0;
word tablepointer=0;
byte slockdata[768];
word subblocks=0;
word compositesync[7]={ 2450,1225,815,815,815,1225,2450 };
byte slockflag=0;
byte slockflagbits=5;
byte midsync[3]={ 7,255,0 };
word pulseseq[255];
byte saveslockflag=0;
byte savemidsync=0;
word beeps=0;
byte endbeep;
word bump[6]={ 4550,15925,25900,36400,54250,9625 };
struct {
	 byte type;
	 char name[11];
	 word length;
	 word start;
       } header;

word faultybits[8]={ 0,0,0,0,0,0,0,0 };
char errstring[81];

/****************************************************************************/
char tzxver[6]="1.10\0",
     groupname[31],
     descr[31];

byte tzxhead[10]="ZXTape!\x1a\x01\x0a",
     stdspeed[5]={ 0x10,0,0,0,0 },
     turboblock[0x13]={ 0x11,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
     puretone[5]={ 0x12,0,0,0,0 },
     pulses[2]={ 0x13,0 },
     puredata[0xb]={ 0x14,0,0,0,0,0,0,0,0,0,0 },
     pause[3]={ 0x20,0,0 },
     groupstart[2]={ 0x21,0 },
     groupend[1]={ 0x22 },
     loopstart[3]={ 0x24,0,0 },
     loopend[1]={ 0x25 },
     text[2]={ 0x30,0 };


/****************************************************************************/

void CheckSyntax(void);
void Error(byte);
void InvalidSwitch(char* invswitch);
void Instructions(void);
void End(void);

byte CreateFile(char*);
byte OpenFile(char*);
void CloseFiles(void);
void Flush(fdata*);
void ResetTempFile(void);
void PushFilePointers(void);
void PopFilePointers(void);
void RemoveFilePointers(void);
void RecoverFilePointers(void);


byte GetData(fdata*);
void PutData(fdata*,byte,word);

void CheckVOCHeader(void);
void ReadVOCBlock(void);
void ProcessVOCData(void);
void ProcessContinue(void);
void ProcessSilence(void);
void ProcessMarker(void);
void ProcessASCII(void);
void ProcessExtended(void);
void SetTimings(void);
void TuneRates(wdata*);

dword FindEdge1(void);
dword FindEdge2(void);
dword PilotSearch(word);
dword SyncSearch(void);
dword DataRead(double,double,word);
dword FindPause(void);

word ReadNormalBlock(void);

dword ReadBleeploadGroup(void);
word ReadBleeploadBlock(void);
void StoreBleeploadCode(word);
void GetBleeploadInfo(word);

word FindAlkatrazLoader1(word);
dword ReadAlkatrazBlock(void);

word ReadSpeedlock13FirstBlock(void);
word ReadSpeedlock47FirstBlock(void);
void FindLoadAddress(word);
void DecodeSpeedlockTable(word,byte);
void CompositeSyncSearch(void);
void ReadSpeedlockFlag(byte,byte);
byte MidSyncSearch(void);
void DecryptToneSearch(void);
void WriteDecryptTone(void);
word BeepSearch(word);
byte MirrorByte(byte);
void SearchDataFrequencies(void);

void PrintBlockInfo(word,dword);

void WriteTZXHeader(void);
void WriteTZXNormalBlock(word);
void WriteTZXTurboBlock(dword,byte,dword);
void WriteTZXTone(word,word);
void WriteTZXPulseSequence(byte);
void WriteTZXPureData(byte,dword);
void WriteTZXPause(word);
void WriteTZXGroupStart(void);
void WriteTZXGroupEnd(void);
void WriteTZXLoopStart(word);
void WriteTZXLoopEnd(void);
void WriteTZXText(void);

/***************************************************************************/

byte CreateFile(char* filename)
{
  int handle;
  handle=open(filename,O_CREAT | O_TRUNC | O_RDWR,0666);
  if (handle==-1) { Error(2); }
  return handle;
}

byte OpenFile(char* filename)
{
  int handle;
  handle=open(filename,O_RDONLY);
  if (handle==-1) Error(1);
  return handle;
}

void CloseFiles(void)
{
  close(inpfile.handle);
  if (outfile.bufpoint) Flush(&outfile);
  close(outfile.handle);
  fclose(logfile);
}

void Flush(fdata* record)
{
  write((*record).handle,(*record).buffer,(*record).bufpoint);
}

void ResetTempFile(void)
{
  if (tempfile.bufpoint) Flush(&tempfile);
  lseek(tempfile.handle,0,SEEK_SET);
  tempfile.bufpoint=0;
}

void PushFilePointers(void)
{
  stackitem *pointer;
  byte item=stack1.busy+stack2.busy+stack3.busy+stack4.busy+stack5.busy+1;
  if (item>5) item=5;
  switch (item)
	 {
	   case 1 : pointer=&stack1; break;
	   case 2 : pointer=&stack2; break;
	   case 3 : pointer=&stack3; break;
	   case 4 : pointer=&stack4; break;
	   case 5 : pointer=&stack5; break;
	 }
  (*pointer).busy=1;
  (*pointer).savedir=direction;
  (*pointer).saverev=revsamples;
  (*pointer).savelast=lastsample;
  (*pointer).savebtype=blocktype;
  (*pointer).savepos=filepos;
  (*pointer).savetotal=totalread;
  (*pointer).saveprocess=processed;
  (*pointer).saveblocklen=blocklength;
}

void PopFilePointers(void)
{
  stackitem *pointer;
  byte item=stack1.busy+stack2.busy+stack3.busy+stack4.busy+stack5.busy;
  switch (item)
	 {
	   case 0 :
	   case 1 : pointer=&stack1; break;
	   case 2 : pointer=&stack2; break;
	   case 3 : pointer=&stack3; break;
	   case 4 : pointer=&stack4; break;
	   case 5 : pointer=&stack5; break;
	 }
  (*pointer).busy=0;
  direction=(*pointer).savedir;
  revsamples=(*pointer).saverev;
  lastsample=(*pointer).savelast;
  blocktype=(*pointer).savebtype;
  filepos=(*pointer).savepos;
  totalread=(*pointer).savetotal;
  processed=(*pointer).saveprocess;
  blocklength=(*pointer).saveblocklen;
}

void RemoveFilePointers(void)
{
  stackitem *pointer;
  byte item=stack1.busy+stack2.busy+stack3.busy+stack4.busy+stack5.busy;
  switch (item)
	 {
	   case 1 : pointer=&stack1; break;
	   case 2 : pointer=&stack2; break;
	   case 3 : pointer=&stack3; break;
	   case 4 : pointer=&stack4; break;
	   case 5 : pointer=&stack5; break;
	 }
  (*pointer).busy=0;
}

void RecoverFilePointers(void)
{
  stackitem *pointer;
  byte item=stack1.busy+stack2.busy+stack3.busy+stack4.busy+stack5.busy;
  switch (item)
	 {
	   case 0 :
	   case 1 : pointer=&stack1; break;
	   case 2 : pointer=&stack2; break;
	   case 3 : pointer=&stack3; break;
	   case 4 : pointer=&stack4; break;
	   case 5 : pointer=&stack5; break;
	 }
  (*pointer).busy=1;
}

/***************************************************************************/

void CheckSyntax(void)
{
  byte params=0,files=0,fnamelen;
  char *fstoppos,*bslashpos;
  printf("\n-=[ MAKETZX v%s ]=- Copyright (C) 1998 Stefano Donati of RAMSOFT\n\n",version);  if (ARGC<1) { Instructions(); }
  for (params=1; params<ARGC; params++)
      {
	if ((ARGV[params][0]=='/') || (ARGV[params][0]=='-'))
	{
	  if (strlen(ARGV[params])>2) Instructions();
	  strnset(ARGV[params],' ',1);
	  ARGV[params]=strlwr(ARGV[params]);
	  switch (ARGV[params][1])
		 {
		   case 'a' : if (slocktype || normal || bleep || soft || zeta) Error(13);
			      alk=1; break;
		   case 'b' : if (slocktype || normal || alk || soft || zeta) Error(13);
			      bleep=1; break;
		   case 'f' : if (alk || normal || bleep || slocktype || zeta) Error(13);
		   case 'k' : params++; blockskip=atoi(ARGV[params]); break;
		   case 'l' : loggen=1; break;
		   case 'n' : if (alk || slocktype || bleep || soft || zeta) Error(13);
			      normal=1; break;
		   case 'q' : hifi=0; break;
		   case 'r' : tzxhead[9]=0x01; tzxver[2]='0'; tzxver[3]='1'; break;
		   case 's' : params++;
			      if ((atoi(ARGV[params])<1) || (atoi(ARGV[params])>7))
				 {
				   printf("ERROR - Speedlock type %s does not exist!\n\n",ARGV[params]);
				   exit(1);
				 }
			      if (alk || normal || bleep || soft || zeta) Error(13);
			      slocktype=atoi(ARGV[params]); break;
		   case 'x' : hex=1; break;
		   case 'z' : if (alk || slocktype || bleep || soft || normal) Error(13);
			      zeta=1; break;
		   default : InvalidSwitch(ARGV[params]);
		 }
	}
	else {
	       files++;
	       switch (files)
		      {
			case 1 : strcpy(inpfile.name,ARGV[params]); break;
			case 2 : strcpy(outfile.name,ARGV[params]); break;
			case 3 : Instructions(); break;
		      }
	     }
      }
  fstoppos=strrchr(inpfile.name,'.');
  bslashpos=strrchr(inpfile.name,'\\');

  //printf("NAME='%s' P=%x BSL=%x\n",inpfile.name,inpfile.name,bslashpos);

  if (!files) Instructions();
  if (files==1)
     {
       if (bslashpos!=NULL) fnamelen=strlen(inpfile.name)-((int)bslashpos-(int)inpfile.name)-1;
       else fnamelen=strlen(inpfile.name);
       strcpy(outfile.name,strncpy(outfile.name,strrev(inpfile.name),fnamelen));
       strrev(inpfile.name); strrev(outfile.name);
       if (fstoppos!=NULL)
	  {
	    outfile.name[(int)strrchr(outfile.name,'.')-(int)outfile.name]='\0';
	  }
       else // strcat(strupr(inpfile.name),".VOC");
            strcat(inpfile.name,".voc");
       // strcat(strupr(outfile.name),".TZX");
       strcat(outfile.name,".tzx");
     }
  else if (fstoppos==NULL) strcat(inpfile.name,".voc");
}

void Error (byte errornumber)
{
  printf("\nERROR - ");
  switch (errornumber)
	 {
	   case 1  : printf("Could not open input file\n\n"); break;
	   case 2  : printf("Could not create output file\n\n"); break;
	   case 3  : printf("Could not read from input file\n\n"); break;
	   case 4  : printf("Could not write to output file\n\n"); break;
	   case 5  : printf("Input file is corrupted or in a wrong format\n\n"); break;
	   case 6  : printf("Extended block contains nonsense data\n\n"); break;
	   case 7  : printf("Found a block at a different sample rate\n\n"); break;
	   case 8  : printf("Tape loading error at sample %lu\n",totalread-lastpulse-currpulse-1);
		     printf("        Probable cause: %s\n\n",errstring); break;
	   case 9  : printf("Unexpected end of input file\n\n"); break;
	   case 10 : printf("Not enough memory for ");
		     if (zeta) printf("ZetaLoad");
		     else printf("Speedlock");
		     printf(" block 1\n\n"); break;
	   case 11 : printf("Cannot determine ");
		     if (zeta) printf("ZetaLoad");
		     else printf("Speedlock");
		     printf(" block 1 loading address\n\n"); break;
	   case 12 : printf("Bleepload block missing\n\n"); break;
	   case 13 : printf("Too many loading standards specified!\n\n"); break;
	   case 14 : printf("Loader is probably not Bleepload\n\n"); break;
	 }
  if (errornumber!=4)
     {
       CloseFiles();
       if (tempfile.bufpoint) Flush(&tempfile);
     }
  close(tempfile.handle);
  remove(tempfile.name);  /* for beta testing comment this line */
  exit(1);
}

void InvalidSwitch(char* invswitch)
{
//  strnset(invswitch,'/',1);
  printf("ERROR - Invalid switch:%s\n\n",invswitch);
  exit(1);
}


void Instructions(void)
{
  printf("Translates VOC files in TZX format for ZX Spectrum emulators\n\n");
  printf("Syntax: MAKETZX <inputfile> [outputfile] [switches]\n\n");
  printf("Switches:\n\n");
  printf("   -n: operate in normal mode         -s n: use Speedlock type n algorithm\n");
  printf("   -a: use Alkatraz algorithm         -k n: skip n blocks before considering\n");
  printf("   -f: use Softlock algorithm               specific loaders\n");
  printf("   -b: use Bleepload algorithm        -x: display file informations in hex\n");
  printf("   -z: use ZetaLoad algorithm         -q: disable hi-fi reproduction tones\n");
  printf("   -l: enable logfile generation      -r: use TZX revision 1.01\n\n");
/*printf("          -digital : use Digital Integrations algorithm and timings\n");*/
  exit(0);
}

void End(void)
{
  if (tempfile.bufpoint) Flush(&tempfile);
  CloseFiles();
  remove(tempfile.name);
  printf("\nDone!\n\n");
  exit(0);
}

/***************************************************************************/

byte GetData(fdata* record)
{
  byte value;                       /* WHAT A MESS!!!!! */
  (*record).bufpoint&=0x3fff;
  if (!((*record).bufpoint)) (*record).xferred+=read((*record).handle,(*record).buffer,sizeof((*record).buffer));
  value=(*record).buffer[(*record).bufpoint];
  (*record).bufpoint++;
  filepos++;
  if (filepos>inpfile.length && blocktype) blocktype=0;
  return value;
}

void PutData(fdata* record,byte value,word dimension)
{
  word count;                                  /* WHAT A MESS!!!!! */
  for (count=0; count<dimension; count++)
      {
	(*record).buffer[((*record).bufpoint)]=value;
	(*record).bufpoint++;
	(*record).bufpoint&=0x3fff;
	if (!(*record).bufpoint)
	   (*record).xferred+=write((*record).handle,(*record).buffer,sizeof(*record).buffer);
      }
}

byte Ehi(dword number)
{ return (byte)(number/65536); }

byte Hi(dword number)
{ return (byte)((number-(65536*Ehi(number)))/256); }

byte Lo(dword number)
{ return (byte)(number-(256*Hi(number))); }


/***************************************************************************/

void CheckVOCHeader(void)
{
  byte vochdr[26]="Creative Voice File\x1a\x1a\x0\x0a\x01\x29\x11",
       count;
  dword totalsamp=0;
  for (count=0; count<sizeof(vochdr); count++)
      {
	if (GetData(&inpfile)!=vochdr[count])
	   Error(5);
      }
  printf("Checking input file integrity...");
  while (!blocktype || !eof(inpfile.handle))
	{
	  ReadVOCBlock();
	  printf("\x0d");
	  printf("Checking input file integrity...");
	  if (!blocktype) break;
	  else {
		 byte ratio=0;
		 totalsamp+=blocklength;
		 if (blocktype!=8 && blocktype!=3) filepos+=blocklength;
		 lseek(inpfile.handle,filepos,SEEK_SET);
		 if (filepos>inpfile.length)
		    {
		      blocktype=0;
		      totalsamp-=(filepos-inpfile.length);
		      filepos=inpfile.length;
		    }
		 inpfile.bufpoint=0;
		 ratio=(totalsamp*100)/inpfile.length;
		 if (!blocktype || eof(inpfile.handle)) break;
/*                  {
		      printf("done\n");
		      printf("Total file length: %lu bytes (%lu
		      samples)\n\n",filepos,totalsamp);
		      if (blocktype && eof(inpfile.handle))
		      printf("***WARNING*** - Unexpected end of file!\n\n");
		    } */
		 else printf("%d%%",ratio);
	       }
	}
/*  if (eof(inpfile.length) && blocktype)
     {
       Error(9);
     } */
  printf("done\n");
  printf("Total file length: %lu bytes (%lu samples)\n\n",filepos,totalsamp);
  if (blocktype && eof(inpfile.handle)) printf("***WARNING*** - Unexpected end
  of file!\n\n");
  lseek(inpfile.handle,26,SEEK_SET);
  filepos=26;
  blocklength=processed=inpfile.bufpoint=0;
}


byte ReadVOCData(void)
{
  if (!(blocklength-processed))
  do {
       processed=0;
       ReadVOCBlock();
       if (!blocktype) return 0;
     }
  while (blocktype==8 || blocktype==3);
  processed++;
  if (blocktype==8 || blocktype==3) processed+=3;
  totalread++;
  return(GetData(&inpfile));
}

void ReadVOCBlock(void)
{
  blocktype=GetData(&inpfile);
  if (!blocktype) return;
  if (blocktype>8)
     Error(5);
  switch (blocktype)
	 {
	 case 1  : ProcessVOCData(); break;
	 case 2  : ProcessContinue(); break;
	 case 3  : ProcessSilence(); break;
	 case 4  : ProcessMarker(); break;
	 case 5  : ProcessASCII(); break;
	 case 8  : ProcessExtended(); break;
	 default : printf("\nSorry, block type %d of VOC files is not yet supported\n\n",blocktype);
		   CloseFiles();
		   exit(1);
	 }
}

void ProcessVOCData(void)
{
  byte brate;
  word timeconst;
  dword data;
  blocklength=(byte)GetData(&inpfile);
  blocklength+=(word)(256*GetData(&inpfile));
  blocklength+=(dword)(65536*GetData(&inpfile));
  blocklength-=2;
  brate=GetData(&inpfile);
  compression=GetData(&inpfile);
  if (compression) { printf("\nSorry, compressed blocks are not yet supported\n\n"); }
  timeconst=65536-(word)(brate*256);
  if (brate>0xd3) timeconst-=0x54;
  samprate=(double)(256000000/timeconst);
  SetTimings();
}

void ProcessContinue(void)
{
  blocklength=(byte)GetData(&inpfile);
  blocklength+=(word)(256*GetData(&inpfile));
  blocklength+=(dword)(65536*GetData(&inpfile));
}

void ProcessSilence(void)
{
  byte silrate;
  word sillength,timeconst;
  blocklength=(byte)GetData(&inpfile);
  blocklength+=(word)(256*GetData(&inpfile));
  blocklength+=(dword)(65536*GetData(&inpfile));
  sillength=(byte)GetData(&inpfile);
  sillength+=(word)(256*GetData(&inpfile));
  silrate=GetData(&inpfile);
  timeconst=65536-(word)(silrate*256);
  silence=sillength/((double)256000000/timeconst)*1000;
}

void ProcessMarker(void)
{
  blocklength=(byte)GetData(&inpfile);
  blocklength+=(word)(256*GetData(&inpfile));
  blocklength+=(dword)(65536*GetData(&inpfile));
}

void ProcessASCII(void)
{
  blocklength=(byte)GetData(&inpfile);
  blocklength+=(word)(256*GetData(&inpfile));
  blocklength+=(dword)(65536*GetData(&inpfile));
}

void ProcessExtended(void)
{
  word timeconst;
  blocklength=(byte)GetData(&inpfile);
  blocklength+=(word)(256*GetData(&inpfile));
  blocklength+=(dword)(65536*GetData(&inpfile));
  timeconst=GetData(&inpfile);
  timeconst+=256*(GetData(&inpfile));
  if (GetData(&inpfile)) Error(6);
  if (GetData(&inpfile))
     {
       printf("\nSorry, stereo VOC files are not yet supported\n\n");
       CloseFiles();
       exit(1);
     }
  SetTimings();
}

void SetTimings(void)
{
  TuneRates(&pilot);
  TuneRates(&sync);
  TuneRates(&bit0);
  TuneRates(&bit1);
}

void TuneRates(wdata* record)
{
 if (samprate && (*record).freq)
    {
      (*record).tstates=(word)(3500000/(2*(*record).freq));
      (*record).samples=(double)(samprate/(*record).freq); //-((*record).freq/100*correction)));
      (*record).rfreq=(float)(samprate/(*record).samples);
    }
}

/***************************************************************************/

dword FindEdge1(void)
{
  dword samples=revsamples;
  byte curr,last,wavdir=direction,loopbreak=0;
  revsamples=0;
  curr=last=lastsample;
  diff=(currpulse-lastpulse);
  lastpulse=currpulse;
/****************************************************************************/
  do {
       byte delta,maxrev;
       last=curr;
       curr=ReadVOCData();
       if (!blocktype) return samples;
       if (abs(curr-last)>=128) { samples++; loopbreak=1; }
       else if (!(curr-last)) { samples++; loopbreak=0; }
	    else {

		   wavdir=(curr-last>0);
		   if (wavdir && !direction) lastpeak=last;
		   delta=(abs(128-lastpeak)/6); if (delta>4) delta=4;
		   maxrev=2-(abs(lastpeak-128)<7);
		   if (maxrev>2) maxrev=2; if (!maxrev) maxrev=1;
		   if (wavdir!=direction)
		      {
			revsamples++;
			if (revsamples>maxrev) { wavdir^=1; loopbreak=1; }
		      }
		   else {
			  samples+=(revsamples+1);
			  if (revsamples) revsamples=0;
			  if (abs(curr-last)<128-delta*4 && (wavdir==direction) || (abs(curr-last)<=delta && wavdir==direction) || curr==last) loopbreak=0;
			  else loopbreak=1;
			}
		 }
     }
 while (!loopbreak);
  direction^=1;
  lastsample=curr;
  currpulse=samples;
  return samples;

/****************************************************************************/
/*  if (lastsample>128) while (curr>128)
			    {
			      last=curr;
			      samples++;
			      curr=ReadVOCData();
			      if (!blocktype) return samples;
			      if (abs(last-curr)>=128 || !(last-curr)) curr=240*(curr>128)+15*(curr<=128);
			      else if (curr>127) curr=240;
				   else curr=16;
			    }
  else while (curr<=128)
	     {                           // Routine 2
	       last=curr;                // For squared and non squared files
	       samples++;
	       curr=ReadVOCData();
	       if (!blocktype) return samples;
	       if (abs(last-curr)>=128 || !(last-curr)) curr=240*(curr>128)+15*(curr<=128);
	       else if (curr>127) curr=240;
		    else curr=16;
	     }
  lastsample=curr;
  currpulse=samples;
  return samples; */
/****************************************************************************/
/*  if (lastsample>128) while (last>128)
			    {
			      samples++;
			      last=ReadVOCData();
			      if (!blocktype) return samples;
			      last=240*(last>128)+15*(last<=128);
			    }
  else while (last<=128)                    // Routine 1
	     {                              // For squared files only
	       samples++;
	       last=ReadVOCData();
	       if (!blocktype) return samples;
	       last=240*(last>128)+15*(last<=128);
	     } */
//  peak=last;
/*  lastsample=last;
  currpulse=samples;
  return samples;*/
}

dword FindEdge2(void)
{
  byte good=0;
  dword hp1=0,hp2=0,total=0;

  if (FindingPause)
     {
       hp1=FindEdge1();
       if (!blocktype)
       if (!FindingPause && hp1>(int)(samprate/500))
	  {
	    mspause=(hp1/samprate)*1000;
	    hp2=hp1=0;
	    EndBlock=1;
	    return hp2;
	  }
       else return hp1;
       hp2=FindEdge1();
       if (!FindingPause && hp2>(int)(samprate/500))
	  {
	    mspause=((hp2-hp1)/samprate)*1000;
	    hp2=hp1;
	    EndBlock=1;
	  }
     }
  else {
	 do {
	      hp1+=FindEdge1();
	      //
	      if (!blocktype)
	      if (!FindingPause && hp1>(int)(samprate/500))
		 {
		   mspause=(hp1/samprate)*1000;
		   hp2=hp1=0;
		   EndBlock=1;
		   return hp2;
		 }
	      else return hp1;
	      //
	      if (currpulse<2 && bit0.samples>3)
		 {
		   PushFilePointers();
		   FindEdge1();
		   if (currpulse<2) { RemoveFilePointers(); good=0; }
		   else {
			  PopFilePointers();
			  lseek(inpfile.handle,filepos,SEEK_SET);
			  inpfile.bufpoint=0;
			  good=1;
			}
		 }
	      else good=1;
	    }
	 while (!good && blocktype);
	 do {
	      hp2+=FindEdge1();
	      //
	      if (!FindingPause && hp2>(int)(samprate/500))
		 {
		   mspause=((hp2-hp1)/samprate)*1000;
		   hp2=hp1;
		   EndBlock=1;
		 }
	      //
	      if (currpulse<2 && bit0.samples>3)
		 {
		   PushFilePointers();
		   FindEdge1();
		   if (currpulse<2) { RemoveFilePointers(); good=0; }
		   else {
			  PopFilePointers();
			  lseek(inpfile.handle,filepos,SEEK_SET);
			  inpfile.bufpoint=0;
			  good=1;
			}
		 }
	      else good=1;
	    }
	  while (!good && blocktype);

       }
  total=(hp1+hp2);
  return total;
}

dword ClickingPilotSearch(void)
{
  byte pulsenum[255],pulsecount,tpointer=0;
  word minplen=samprate/(pilot.freq+150);
  dword hpulses;
  ClickSearch=1;
  ClickingPilotOK=0;
  printf("Loading clicking pilot... \x0d");
  do {
       hpulses=80;
       PilotSearch(hpulses/2);
       hpulses+=(SyncSearch()+1);
       if (SyncLoadOK)
	  {
	    if (hpulses>254) return hpulses;
	    if (hpulses&1)
	       if (lastpulse*2<minplen) hpulses--;
	    pulsenum[tpointer]=hpulses;
	    tpointer++;
	    if (tzxhead[9]<0x0a)
	       {
		 WriteTZXTone(pilot.tstates,hpulses);
		 pulseseq[0]=pulseseq[1]=sync.tstates;
		 WriteTZXPulseSequence(2);
	       }
	  }
       else {

	      word p250num=0;
	      word syncp;
	      int diff=hpulses-(pulsenum[tpointer-(tpointer+1)/8]);
	      hpulses-=diff;
	      if (!diff) FindEdge1();
	      if (!blocktype) return 0;
	      pulsenum[tpointer]=hpulses;
	      pulsecount=++tpointer;
	      if (tzxhead[9]>=0x0a)
		 {
		   byte count;
		   WriteTZXLoopStart(7);
		   for (count=tpointer-(tpointer+1)/8; count<tpointer; count++)
		       {
			 WriteTZXTone(pilot.tstates,pulsenum[count]);
			 pulseseq[0]=pulseseq[1]=sync.tstates;
			 WriteTZXPulseSequence(2);
		       }
		   WriteTZXLoopEnd();
		   for (count=tpointer-(tpointer+1)/8; count<tpointer-1; count++)
		       {
			 WriteTZXTone(pilot.tstates,pulsenum[count]);
			 pulseseq[0]=pulseseq[1]=sync.tstates;
			 WriteTZXPulseSequence(2);
		       }

		 }
	      WriteTZXTone(pilot.tstates,hpulses);
	      pulseseq[0]=pulseseq[1]=3500000/(2*555);
	      WriteTZXPulseSequence(2);
	      ClickingPilotOK=1;
	      for (tpointer=0; tpointer<pulsecount; tpointer++)
		  { if (pulsenum[tpointer]==250) p250num++; }
	      if (p250num>(pulsecount/8)-1) slockflagbits=6;
	    }

     }
  while (SyncLoadOK);
  ClickSearch=0;
  return hpulses;
}

dword PilotSearch(word pulses)
{
  byte tolerance=pilot.samples-samprate/(pilot.freq+200-(pilot.freq/100*correction));
  word count;
  dword skipped1=0,skipped2=0,hpulse,pulse;
  wmaxcount=wmincount=wmax=wmin=0;
  PilotLinkingOK=0;
  if (spdchk && pilot.freq==808)
     do {
	  word pfreq;
	  CheckingSpeed=1;
	  PushFilePointers();
	  pfreq=BeepSearch(16);
	  if (!pfreq && !blocktype)
	     {
	       PopFilePointers();
	       lseek(inpfile.handle,filepos,SEEK_SET);
	       inpfile.bufpoint=0;
	       ecf=1; break;
	     }
	  PopFilePointers();
	  lseek(inpfile.handle,filepos,SEEK_SET);
	  inpfile.bufpoint=0;
	  ecf=(double)808/pfreq;
	  if (abs(ecf)<.10) ecf=1;
	  CheckingSpeed=0;
	}
     while (abs(ecf-1)>.20);
  if (!FindingPause && !ClickSearch) printf("Searching pilot...     \x0d");
  if (spdchk) pilot.samples*=ecf;
  do {
       do {
	    hpulse=FindEdge1();
	    if (!blocktype) return skipped1+skipped2+hpulse;
	    skipped1+=hpulse;
	  }
       while (hpulse<(pilot.samples/2)-tolerance || hpulse>(pilot.samples/2)+tolerance);
       skipped2=0;
       PushFilePointers();
       for (count=0; count<pulses; count++)
	   {
	     skipped2+=(pulse=FindEdge2());
	     if ((pulse<pilot.samples-(2*tolerance) || pulse>pilot.samples+(2*tolerance)) && !tonesearch)
		{ RemoveFilePointers(); break; }
/*           if (tapespeedcheck)
		{
		  if (pulse==wmax-(wmax-wmin)/2 && wmax!=0) wmaxcount++;
		  else if (pulse==wmin+(wmax-wmin)/2 && wmin!=0) wmincount++;
		  if (pulse<wmin) wmincount=1;
		  else if (pulse>wmax) wmaxcount=1;
		  if (wmax==0) wmax=pulse;
		  else wmax=(pulse*(pulse>wmax))+(wmax*(pulse<=wmax));
		  if (wmin==0) wmin=pulse;
		  else wmin=(pulse*(pulse<wmin))+(wmin*(pulse>=wmin));
		} */
	   }
       if (count==pulses) PilotLinkingOK=1;
       else { skipped1+=skipped2; skipped2=0; }
     }
  while (count!=pulses);
  if (!FindingPause) pilotlength+=skipped2+hpulse;
  if (!FindingPause && !ClickSearch) printf("Loading pilot...       \x0d");
  if (FindingPause)
     {
       PopFilePointers();
       lseek(inpfile.handle,filepos,SEEK_SET);
       inpfile.bufpoint=0;
       RemoveFilePointers();
       PushFilePointers();
     }
  else RemoveFilePointers();
  return skipped1-hpulse;
}

dword SyncSearch(void)
{
  byte minsmp=(byte)(samprate/((pilot.freq+30)*4)),
       middle=(byte)((float)(samprate/((pilot.freq+50)*2)+.5)),
       maxsmp=(byte)(samprate/(pilot.freq-230)),
       pulseget=0;
  word hwaves=0;
  long pulse;
  float tolerance=(middle-sync.samples)*.75;
  if (spdchk) sync.samples*=ecf;
  SyncLoadOK=0;
     {
       do {
	    do {
		 if (!pulseget) pulse=FindEdge1();
		 else { pulse-=lastpulse; pulseget=0; }
		 if (!pulse || pulse<(minsmp-tolerance)/2 || pulse>maxsmp/2)
		    { mspause+=(pulse/samprate)*1000; return hwaves; }
		 if (abs(pulse-lastpulse)>diff*5 && abs(pulse-lastpulse) && abs(diff)>tolerance) pulse+=(diff*(abs(diff)<5));
		 if (pulse<0)
		 pulse-=diff;
		 hwaves++;
		 pilotlength+=pulse;
	       }
	    while (pulse>(byte)((double)(middle+tolerance)/2+.5) || (pulse/2+lastpulse/2<middle/2 && diff>minsmp/2));
	    if (abs(pulse-lastpulse)>middle/2 && abs(lastpulse-middle)>tolerance && pulse)
	       {
		 double sum=pulse+lastpulse;
//               if (!pulse) sum-=diff;
		 pulse=sum*.3; lastpulse=sum-pulse;
	       }
	    if (pulse+lastpulse>middle || (abs(lastpulse-(byte)pilot.samples/2)<=tolerance && pulse>=sync.samples/2) || sync.samples<2)// || abs(lastpulse-(byte)sync.samples)>1) //lastpulse>sync.samples)
	       {
		 pilotlength-=pulse;
		 if (!pulseget) { pulse+=FindEdge1(); pulseget=1; }
		 else { pulseget=0; pulse+=lastpulse; }
	       }
	    else { pulse+=lastpulse; pulseget=0; }
//          if (ClickSearch && !(hwaves%2) && abs(pulse-middle)<tolerance) pulse=middle;
	  }
       while (pulse<minsmp || pulse>middle);
       SyncLoadOK=1;
       return hwaves;
     }
}

dword DataRead(double bit0,double bit1,word explen)
{
  byte value,count,lastval,tolerance,maxpulse;/*=bit1+bit0-tolerance;*/
  word pulse;
  dword amount=0;
  double middle;
  if (spdchk) { bit0*=ecf; bit1*=ecf; }
  middle=(double)(bit1+bit0)/2;
//  if (bit0-bit1+2*tolerance) { bit1++; tolerance++; }
/*  if (maxpulse>pilot.samples)*/
  maxpulse=bit1+(pilot.samples-bit1)/2+1;
  tolerance=(bit1-bit0)/2;
  if (maxpulse>(byte)bit1+(byte)bit0) maxpulse=bit1+bit0-tolerance;
  if (!subblocks && !slocktype || raw) checksum=0;
  for (count=0; count<8; count++) faultybits[count]=0;
  probend=0;
  EndBlock=0;
  DataLoadOK=0;
  printf("Reading data...            \x0d");
  if (!explen)
     {
       do {
	    PushFilePointers();
	    value=0;
	    for (count=0; count<8; count++)
		{
//                byte good=1;
		  pulse=0;
		  if (EndBlock) break;

/*                do {
		       pulse+=FindEdge1();
		       if (currpulse<2) good=0;
		       else good=1;
		     }
		  while (!good && blocktype);
		  do {
		       pulse+=FindEdge1();
		       if (currpulse<2) good=0;
		       else good=1;
		     }
		  while (!good && blocktype); */

//                do pulse+=FindEdge2();
//                while (pulse<bit0-tolerance && blocktype);

		     do {
			  pulse=FindEdge2();
//                        if (pulse<bit0-tolerance && pulse-diff>0 && abs(diff)<5)
//                        pulse-=diff;
			}
		     while (pulse<bit0-tolerance && blocktype);


		  if ((abs(pulse-(int)(middle+.5))<2 && abs(pulse-2*currpulse)>2) || pulse>maxpulse)
		     {
		       if (currpulse+lastpulse>maxpulse)
			  {
			    if (!count)
			       {
				 mspause+=(pulse/samprate)*1000;
				 EndBlock=1;
				 break;
			       }
			    if (count==7)
			       {
				 mspause+=((pulse-(2*lastpulse))/samprate)*1000;
				 pulse=2*lastpulse;
				 EndBlock=1;
			       }
			    else if ((slocktype>1 && slocktype<5) && blocks>blockskip+1)
				    {
				      PopFilePointers();
				      //RecoverFilePointers();
				      lseek(inpfile.handle,filepos,SEEK_SET);
				      inpfile.bufpoint=0;
				      EndBlock=1; break;
				    }
				 else { EndBlock=1; break; }
			  }
			else {
			       if (loggen) fprintf(logfile,"?- Ambiguous pulse found: %u samples,",pulse);
			       pulse=((currpulse+lastpulse)/2+pulse-middle+(lastpulse<currpulse))*2;
			       if (loggen) {
					     fprintf(logfile," changed to %u (%d)\n",pulse,pulse>middle);
					     fprintf(logfile,"   Position: block %ld, byte %lu, bit %ld, sample %lu\n\n",(dword)blocks,(dword)amount,(dword)7-count,(dword)totalread-pulse);
					   }
			       faultybits[7-count]++;
			      }
		     }

	       /*         if (pulse>bit1+tolerance || pulse>pilot.samples)
		     {
		       if (count==7) {
				       mspause+=((pulse-(2*lastpulse))/samprate)*1000;
				       pulse=2*lastpulse;
				       EndBlock=1;
				     }
		       else if ((slocktype>1 && slocktype<5) && blocks>blockskip+1)
			       {
				 PopFilePointers();
				 lseek(inpfile.handle,filepos,SEEK_SET);
				 inpfile.bufpoint=0;
				 EndBlock=1; break;
			       }
			    else { EndBlock=1; break; }
		     } */
		  if (!blocktype && !count)
		     {
		       if ((alk && blocks>2) || (!checksum && amount)) DataLoadOK=1;
		       return amount;
		     }
		  if (pulse>=bit0-tolerance && pulse<=middle) value<<=1;
		  else if (pulse>middle && pulse<=maxpulse) { value<<=1; value|=0x1; }
		}
	    if (count==8)
	       {
		 amount++;
		 if (amount==1) flag=value;
		 chksum=lastval=value;
		 lastval=value;
		 checksum^=value;
		 if (!checksum) probend=amount;
		 PutData(&tempfile,value,1);
	       }
	    RemoveFilePointers();
	  }
       while (!EndBlock);
//       RecoverFilePointers();
       if ((!checksum && amount) || ((alk || (blocks>blockskip && blockskip && !raw)) || (slocktype>1 && slocktype<5 && blocks>blockskip+1 && blockskip)))
	  {
	    byte bitpoint;
	    byte fbits=0;
	    DataLoadOK=1;
	    if (loggen)
	       {
		 fprintf(logfile,"!- Block %d",blocks);
		 if (subblocks) fprintf(logfile," -> Sub-block %d",subblocks);
		 if (alk)
		    {
		      if (blocks<blockskip) fprintf(logfile," loaded OK\n");
		      else fprintf(logfile," is Alkatraz and cannot be checked\n");
		    }
		 else fprintf(logfile," loaded OK\n");
	       }
	    for (bitpoint=0; bitpoint<8; bitpoint++)
		{
		  if (faultybits[bitpoint]>1) { fbits++; }
		}
	    if (fbits)
	       {
		 if (loggen)
		    {
		      fprintf(logfile,"\n!- WARNING!!! Several ambiguous pulses were found loading bit");
		      if (fbits>1) fprintf(logfile,"s");
		      fprintf(logfile," ");
		    }
		 for (bitpoint=0; bitpoint<8; bitpoint++)
		     if (faultybits[bitpoint]>1)
			   {
			     if (loggen) fprintf(logfile,"%d",bitpoint);
			     fbits--;
			     if (loggen)
				{
				  if (fbits) fprintf(logfile,",");
				  else fprintf(logfile,".\n");
				}
			   }
		 if (loggen)
		    {
		      fprintf(logfile,"   It's strictly recommended that you check that block by loading it on a real\n");
		      fprintf(logfile,"   ZX Spectrum (better solution) or an emulator.\n\n");
		    }
	       }
	  }
       else if (probend && amount-probend<=2)
	    { amount=probend; checksum=0; DataLoadOK=1; }
	    else {
		   char bitpoint;
		   word meanbits[8]={ 0,0,0,0,0,0,0,0 };
		   byte bit,fbits=0;
		   byte dchecksum=checksum;
		   DataLoadOK=0;
		   if (loggen) fprintf(logfile,"X- Bad checksum in block %d (calculated=%d (#%2X), loaded=%d (#%2X))\n",blocks,dchecksum^lastval,dchecksum^lastval,lastval,lastval);
		   for (bitpoint=7; bitpoint>=0; bitpoint--)
		       {
			 if (faultybits[bitpoint])
			    {
			      //bit=pow(2,bitpoint)
			      bit=(int)(1<<bitpoint);
			      if (dchecksum>bit-1)
				 {
				   meanbits[bitpoint]++;
				   fbits++;
				   dchecksum^=bit*(faultybits[bitpoint]==1);
				 }
			    }
			 if (!dchecksum) break;
		       }
		   if (loggen)
		      {
			if (!fbits) fprintf(logfile,"   No ambiguous bits in that block...try resampling and cross your fingers!!!\n");
			else {
			       fprintf(logfile,"   Ambiguous pulses were found in the block ");
			       if (!dchecksum) fprintf(logfile,"and some of them seemed\n   to be determinant:\n");
			       else fprintf(logfile," but none of them was determinant\n   Please try resampling...\n\n");
			     }
		      }
		 }
       if (!mspause && !subblocks) mspause=(pulse/samprate)*1000;
       if (loggen) fprintf(logfile,"\n+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n\n");
     }
  else {
	 mspause=0;
	 for (amount=0; amount<explen; amount++)
	     {
//             byte good=1;
	       value=0;
	       for (count=0; count<8; count++)
		   {

		     pulse=0;
/*                   do {
			  pulse+=FindEdge1();
			  if (currpulse<2) good=0;
			  else good=1;
			}
		     while (!good && blocktype);
		     do {
			  pulse+=FindEdge1();
			  if (currpulse<2) good=0;
			  else good=1;
			}
		     while (!good && blocktype); */


		     do {
			  pulse=FindEdge2();
//                        if ((currpulse<2 || lastpulse<2) && abs(currpulse-lastpulse)>4)
//                           do pulse+=FindEdge1();
//                           while (currpulse<2);
			}
		     while (pulse<bit0-tolerance && blocktype);

		     if (!blocktype && !count)
			{
			  if (!checksum) DataLoadOK=1;
			  return amount;
			}
		     if (pulse>=bit0-tolerance && pulse<=middle) value<<=1;
		     else if (pulse>middle && pulse<=maxpulse) { value<<=1; value|=0x1; }
			  else { strcpy(errstring,"Pulse too long"); Error(8); }
		   }
	       if (soft && blocks>blockskip) checksum^=MirrorByte(value);
	       else checksum^=value;
	       if (amount==1) flag=value;
	       if (amount==explen-1)
	       chksum=value;
	       if (amount==0x11d2)
	       amount=amount+1-1;
	       PutData(&tempfile,value,1);
	     }
	 if (!checksum || ((slocktype || zeta) && blocks>blockskip && !SpeedEndBlock) ||
	      (soft)) DataLoadOK=1;
	 expectedlength=0;
       }
  return amount;
}

dword FindPause(void)
{
  dword hpulse;
  double detfreq;
  PushFilePointers();
  FindingPause=1;
  printf("Finding pause...     \x0d");
//  do detfreq=BeepSearch(50);
//  while (abs(detfreq-808)>150 && blocktype);
//  if (!blocktype) return 0;
  hpulse=PilotSearch(50)/ecf;
  FindingPause=0;
  if (blocktype)
     {
       PopFilePointers();
       lseek(inpfile.handle,filepos,SEEK_SET);
       inpfile.bufpoint=0;
     }
  printf("                     \x0d");
  if (silence) { mspause+=silence; silence=0; }

  return hpulse;
}

word ReadNormalBlock(void)
{
  word loaded=0;
  pilot.freq=808;
  sync.freq=2450;
  bit0.freq=2044;
  bit1.freq=1022;
  SetTimings();
  DataLoadOK=0;
  pilotlength=0;
  do {
       do {
		printf("Searching pilot...       \x0d");
		PilotSearch(256);
		if (!blocktype) return 0; if (!PilotLinkingOK) { pilotlength=0; break;}
		mspause=0;
		SyncSearch();
		if (!blocktype) return 0; if (!SyncLoadOK) break;
		loaded=DataRead(bit0.samples,bit1.samples,expectedlength);
		if (blocktype && ((slocktype<3 || slocktype>6) || blocks!=blockskip)) mspause+=(FindPause()/samprate)*1000;
		else mspause+=7;
	  }
       while (!DataLoadOK);
     }
  while (!DataLoadOK);
  ResetTempFile();
  if (loaded==19)
     {
       byte count;
       dword savefilepos=filepos;
       ResetTempFile();
       if (GetData(&tempfile)==0)
	  {
	    header.type=GetData(&tempfile);
	    for (count=0; count<10; count++)
		{
		  header.name[count]=GetData(&tempfile);
		  if (header.name[count]<' ') header.name[count]='*';
		}
	    header.name[10]=0;
	    header.length=(byte)(GetData(&tempfile));
	    header.length+=(word)(256*GetData(&tempfile));
	    header.start=(byte)(GetData(&tempfile));
	    header.start+=(word)(256*GetData(&tempfile));
	    expectedlength=(header.length)+2;
	    if (header.type==0) nxtloadpos=header.start=23755;
	    else nxtloadpos=header.start;
	  }
       filepos=savefilepos;
     }
  else {
	 if (header.type>3) header.start=0;
	 header.type=0xf;
       }
  return loaded;
}

dword ReadBleeploadGroup(void)
{
  word bytes;
  dword grandtotal=0;
  do {
       subblocks++;
       header.type=9;
       expectedlength=0x10b;
       grandtotal+=(bytes=ReadBleeploadBlock());
       if (!DataLoadOK) { strcpy(errstring,"Bad mid-sync detection or data error"); Error(8); }
       GetBleeploadInfo(bytes);
       if (bleepblock[0x101]!=subblocks-1) Error(12);
       PrintBlockInfo(nxtloadpos,bytes-2);
       WriteTZXTurboBlock(pilotlength,8,bytes);
     }
  while (bleepblock[0x103]+1!=bleepblock[0x105]);
  pilotlength=0;
  PilotSearch(50);
  if (!blocktype) return grandtotal;
  if (!PilotLinkingOK) pilotlength=0;
  mspause=0;
  SyncSearch();
  if (!SyncLoadOK && (pilotlength/samprate>0.5))
     {
       WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
       mspause+=(FindPause()/samprate)*1000;
       if (mspause) WriteTZXPause(mspause);
     }
  pilotlength=0;
  header.type=0x9f;
  subblocks=0;
  PrintBlockInfo(0,grandtotal);
  return grandtotal;
}

byte SearchBleeploadGroup3(void)
{
  PushFilePointers();
  expectedlength=0x10b;
  searching=1;
  ReadBleeploadBlock();
  searching=0;
  if (!blocktype) return blocktype;
  PopFilePointers();
  lseek(inpfile.handle,filepos,SEEK_SET);
  inpfile.bufpoint=0;
  return DataLoadOK;
}

word ReadBleeploadBlock(void)
{
  word loaded=0;
  pilot.freq=808;
  sync.freq=2450;
  bit0.freq=2044;
  bit1.freq=1022;
  SetTimings();
  pilotlength=0;
  DataLoadOK=0;
//  PushFilePointers();
  do
     do {
//        RemoveFilePointers();
	  pilotlength=0;
	  PilotSearch(50);
	  if (!blocktype) return 0; if (!PilotLinkingOK) { pilotlength=0; break;};
	  mspause=0;
	  SyncSearch();
	  if (!blocktype) return 0;
	  if (!SyncLoadOK)
	     {
	       if (searching) break;
	       if (pilotlength/samprate>0.5) WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
	       mspause+=(FindPause()/samprate)*1000;
	       if (mspause && pilotlength/samprate>0.5) WriteTZXPause(mspause);
	       break;
	     }
	  loaded=DataRead(bit0.samples,bit1.samples,expectedlength);
	  if (blocktype && !searching) mspause+=(FindPause()/samprate)*1000;
	}
     while (!DataLoadOK);
  while (!DataLoadOK);
//  PushFilePointers();
  ResetTempFile();
  return loaded;
}

void StoreBleeploadCode(word length)
{
  word count;
  PushFilePointers();
  if (length>512) Error(14);
  ResetTempFile();
  GetData(&tempfile);
  for (count=0; count<length; count++)
      { bleeploader[count]=GetData(&tempfile); }
  for (count=1; count<length-3; count++)
      {
	if (bleeploader[count]==0x32 && bleeploader[count+1]==0x15 && bleeploader[count+2]==0xff)
	   { expectedlength=0x109+2+bleeploader[count-1]; break;}
      }
  ResetTempFile();
  PopFilePointers();
//  RecoverFilePointers();
}

void GetBleeploadInfo(word length)
{
  word count;
  PushFilePointers();
  ResetTempFile();
  GetData(&tempfile);
  for (count=0; count<length; count++) { bleepblock[count]=GetData(&tempfile); }
  nxtloadpos=bleepblock[0x102]+256*bleepblock[0x103];
  expectedlength=0x109+(bleepblock[0x104+bleepblock[0x104]+4]&7)+3;
  ResetTempFile();
  PopFilePointers();
//  RecoverFilePointers();
}

word ReadSoftlockFirstBlock(void)
{
  dword loaded;
  pilot.freq=808;
  sync.freq=2450;
  bit0.freq=2600;
  bit1.freq=1300;
  SetTimings();
  pilotlength=0;
  DataLoadOK=0;
  while (!DataLoadOK)
	{
	  PilotSearch(256);
	  if (!blocktype) return 0; if (!PilotLinkingOK) { pilotlength=0; return 0;}
	  mspause=0;
	  SyncSearch();
	  if (!blocktype) return 0; if (!SyncLoadOK) return 0;
	  loaded=DataRead(bit0.samples,bit1.samples,expectedlength);
	}
  return loaded;
}

word FindAlkatrazLoader1(word limit)
{
  byte lastval;
  word count,position,offset;
  PushFilePointers();
  ResetTempFile();
  for (count=0; count<limit; count++)
      {
	if ((lastval=GetData(&tempfile))==0xea && count>127) break;
      }
  do { lastval=GetData(&tempfile); }
  while (lastval!=0x11);
  position=GetData(&tempfile);
  position+=256*GetData(&tempfile)+1;
  if (position<61440) offset=0x190;
  else offset=0x200;
  position-=offset;
  ResetTempFile();
  PopFilePointers();
  lseek(inpfile.handle,filepos,SEEK_SET);
  inpfile.bufpoint=0;
  return position;
}

dword ReadAlkatrazBlock(void)
{
  dword loaded;
  pilot.freq=808;
  sync.freq=2450;
  bit0.freq=3100;
  bit1.freq=1550;
  SetTimings();
  pilotlength=0;
  DataLoadOK=0;
  while (!DataLoadOK)
	{
	  PilotSearch(50);
	  if (!blocktype) return 0; if (!PilotLinkingOK) { pilotlength=0; return 0;}
	  mspause=0;
	  SyncSearch();
	  if (!blocktype) return 0; if (!SyncLoadOK) return 0;
	  loaded=DataRead(bit0.samples,bit1.samples,expectedlength);
	  if (blocktype) mspause+=(FindPause()/samprate)*1000;
	}
  return loaded;
}

word ReadSpeedlock13FirstBlock(void)
{
  word loaded;
  pilotlength=0;
  DataLoadOK=0;
  while (!DataLoadOK)
	{
	  PushFilePointers();
	  pilotlength=ClickingPilotSearch();
	  if (!blocktype || !ClickingPilotOK) return 0;
	  mspause=0;
	  ReadSpeedlockFlag(bit0.samples,bit1.samples);
	  if (!blocktype) return 0;
	  loaded=DataRead(bit0.samples,bit1.samples,expectedlength);
	  if (blocktype && blocks<=blockskip+1 || slocktype==1) mspause+=(FindPause()/samprate)*1000;
	}

  return loaded;
}
word ReadSpeedlock47FirstBlock(void)
{
  word loaded;
  pilotlength=0;
  DataLoadOK=0;
  while (!DataLoadOK)
	{

	  do {
	       PilotSearch(32);
	       if (!blocktype) return 0;
	       if (!PilotLinkingOK) { pilotlength=0; }
	     }
	  while (!PilotLinkingOK);
	  mspause=0;
	  do
	     {
	       SyncSearch();
	       if (!blocktype) return 0;
	     }
	  while (!SyncLoadOK);

/*        PilotSearch(32);
	  if (!blocktype) return 0; if (!PilotLinkingOK) { pilotlength=0; return 0; }
	  mspause=0;
	  SyncSearch();
	  if (!blocktype) return 0; if (!SyncLoadOK) return 0; */
	  CompositeSyncSearch();
	  if (!blocktype) return 0;
	  ReadSpeedlockFlag(bit0.samples,bit1.samples);
	  if (!blocktype) return 0;
	  loaded=DataRead(bit0.samples,bit1.samples,expectedlength);
	  if (blocktype && blocks<=blockskip+1) mspause+=(FindPause()/samprate)*1000;
	}

  return loaded;
}

void CompositeSyncSearch(void)
{
  byte count,tolerance;
  dword pulse;
  for (count=0; count<7; count++)
      {
	sync.freq=compositesync[count];
	SetTimings();
	tolerance=sync.samples/3;
	pulse=FindEdge2();
	if (!blocktype) return;
	if (pulse<(int)sync.samples-tolerance || pulse>(int)sync.samples+tolerance)
	   { strcpy(errstring,"Bad sync detection or corrupted composite-sync"); Error(8); }
      }
}

void ReadSpeedlockFlag(byte bit0,byte bit1)
{
  byte count,value=0,tolerance,maxpulse;/*=bit1+bit0-tolerance;*/
  word pulse;
/*  if (maxpulse>pilot.samples) */
  if (spdchk) { bit0*=ecf; bit1*=ecf; }
  tolerance=(bit1-bit0)/2;
  maxpulse=bit1+(pilot.samples-bit1)/2;
  for (count=0; count<slockflagbits+1; count++)
      {
	pulse=FindEdge2();
	if ((pulse<=bit0+tolerance) && (pulse>=bit0-tolerance)) value<<=1;
	else if ((pulse<=maxpulse) && (pulse>=bit1-tolerance)) { value<<=1; value|=0x01; }
	     else if (pulse>maxpulse) { strcpy(errstring,"Pulse too long"); Error(8);}
      }
  value<<=7-slockflagbits;
  slockflag=value;
}

byte MidSyncSearch(void)
{
  byte count1,count2,value,tolerance,maxpulse;
//  byte good=1;
  word pulse;
  double middle;
  bit0.freq=2000; bit1.freq=1000;
  SetTimings();
  if (spdchk) { bit0.samples*=ecf; bit1.samples*=ecf; }
  tolerance=(bit1.samples-bit0.samples)/2;
  middle=(double)(bit1.samples+bit0.samples)/2;
  maxpulse=bit1.samples+(pilot.samples-bit1.samples)/2+1;
  if (maxpulse>(byte)bit1.samples+(byte)bit0.samples) maxpulse=bit1.samples+bit0.samples-tolerance;
  for (count1=0; count1<2; count1++)
      {
	value=1;
	for (count2=0; count2<8; count2++)
	    {
	      pulse=0;

/*                   do {
			  pulse+=FindEdge1();
			  if (currpulse<2) good=0;
			  else good=1;
			}
		     while (!good && blocktype);
		     do {
			  pulse+=FindEdge1();
			  if (currpulse<2) good=0;
			  else good=1;
			}
		     while (!good && blocktype); */


	      pulse=FindEdge2();


//            if ((abs(pulse-(int)(middle+.5))<2 /* && abs(pulse-2*currpulse)>2 */) || pulse>maxpulse)
//            pulse=((currpulse+lastpulse)/2+pulse-middle+(lastpulse<currpulse))*2;
	      if ((pulse<middle) && (pulse>=bit0.samples-tolerance)) value<<=1;
	      else if ((pulse<=bit1.samples+tolerance) && (pulse>=middle)) { value<<=1; value|=0x1; }
	    }
	if (value!=midsync[count1])
	   {
	     PopFilePointers();
	     lseek(inpfile.handle,filepos,SEEK_SET);
	     inpfile.bufpoint=0;
	     return 0;
	   }
      }
  value=0;
  for (count2=0; count2<5; count2++)
      {
	pulse=0;

/*                   do {
			  pulse+=FindEdge1();
			  if (currpulse<2) good=0;
			  else good=1;
			}
		     while (!good && blocktype);
		     do {
			  pulse+=FindEdge1();
			  if (currpulse<2) good=0;
			  else good=1;
			}
		     while (!good && blocktype); */


	pulse=FindEdge2();
	if ((abs(pulse-(int)(middle+.5))<2 /* && abs(pulse-2*currpulse)>2 */) || pulse>maxpulse) pulse=((currpulse+lastpulse)/2+pulse-middle+(lastpulse<currpulse))*2;
	if ((pulse<middle) && (pulse>=bit0.samples-tolerance)) value<<=1;
	else if ((pulse<=bit1.samples+tolerance) && (pulse>=middle)) { value<<=1; value|=0x1; }
      }
  if (value!=midsync[2])
     {
       PopFilePointers();
       lseek(inpfile.handle,filepos,SEEK_SET);
       inpfile.bufpoint=0;
       return 0;
     }
  savemidsync=1;
  WriteTZXPureData(5,3);
  savemidsync=0;
  return 1;
}

word BeepSearch(word minlen)
{
  byte tolerance,noise,nobeep;
  word count,pulsecount;
  dword pulse=0,
	maxlim=samprate/(2*425),
	minlim=samprate/(2*3500);
  double beepfreq,wavelen;
  mspause=0;
  PushFilePointers();
  do {
       noise=0; nobeep=0; wavelen=0; beeplength=0; pulsecount=0;
       do {
	    tolerance=(pulse=FindEdge1())/2;
	    if (!blocktype) return 0;
//new code begins
	    if (!tonesearch && tolerance>4) tolerance=4;
//new code ends
	  }
       while (abs(pulse-lastpulse)>tolerance || pulse<minlim || pulse>maxlim);
       wavelen=(beeplength=pulse)*2; pulsecount=1;
//new code begins
       if (tonesearch)
	  { tolerance=wavelen-samprate/((samprate/wavelen)+400); }
//new code ends
       for (count=0; count<minlen; count++)
	   {
	     pulse=FindEdge2();
	     if (abs(pulse-wavelen)>tolerance)
	     { noise=1; break; }
	     else { beeplength+=pulse; pulsecount+=2; }
	   }
     }
  while (noise || nobeep);
  if (!raw)
     do {
	  pulse=FindEdge1();
	  if (!blocktype) return 0;
	  beeplength+=pulse; pulsecount++;
	}
     while (abs(pulse-wavelen/2)<=tolerance || (abs(pulse-5)<3 && tonesearch));
  beeplength-=pulse;
  wavelen=(float)2*beeplength/pulsecount;
  beepfreq=(samprate/wavelen);
  if (CheckingSpeed)
     {
       PopFilePointers();
       return beepfreq;
     }
  msbeep=(beeplength*1000)/samprate;
  if (msbeep<30 || msbeep>100)
     {
       if (!tonesearch)
	  {
	    PopFilePointers();
	    lseek(inpfile.handle,filepos,SEEK_SET);
	    inpfile.bufpoint=0;
	    mspause+=(FindPause()/samprate)*1000;
	    WriteTZXPause(mspause);
	    endbeep=1;
	  }
       else RemoveFilePointers();
     }
  else {
	 byte count;
	 word beeptstates;
	 if (beepfreq) beeptstates=(word)((float)(3500000/(2*beepfreq))+.5);
	 if (!tonesearch)
	    {
	      WriteTZXTone(beeptstates,beeplength/(wavelen/2));
	      pulses[1]=6;
	      for (count=0; count<6; count++)
		  { pulseseq[count]=bump[count]; }
	      WriteTZXPulseSequence(sizeof(bump));
	    }
	 RemoveFilePointers();
       }
  mspause+=(pulse/samprate)*1000;
  return beepfreq;
}


/*{
  word count;
  byte noise,nobeep;
  byte tolerance=0,tolerance2=0;   //=((175-(samprate/282.22))/samprate)*1000;
  dword pulse1,pulse2;
  dword maxlim=samprate/(2*425);
  dword minlim=samprate/(2*3500);
  word beepfreq;
  float wavelen;
  mspause=0;
  wmincount=wmaxcount=0;
  PushFilePointers();
  do {
       do {
	    dword min,max;
	    wmax=wmin=0;
	    noise=0;
	    beeplength=nobeep=0;
	    do {
		 pulse1=FindEdge1();
		 if (!blocktype) return 0;
		 if (pulse1<minlim || pulse1>maxlim)
		    { noise=1; break; }
	       }
	    while ((pulse1<minlim || pulse1>maxlim));
	  }
       while (noise);
       beeplength=pulse1*2;
       wmax=wmin=beeplength;
       wavelen=(byte)((float)beeplength/2+.5);
       tolerance=(byte)((float)(wavelen*.5)+.5);
       tolerance2=tolerance/2;
       for (count=0; count<minlen; count++)
	   {
	     pulse1=FindEdge1();
	     if (!blocktype) return 0;
	     pulse1=FindEdge1();
	     if (!blocktype) return 0;
	     if ((pulse1<wavelen-tolerance || pulse1>wavelen+tolerance) ||
		(pulse2<wavelen-tolerance || pulse2>wavelen+tolerance) ||
		(wmax-wmin>tolerance))
		{ nobeep=1; break; }
	     else {
		    beeplength+=(pulse1+pulse2);
		    if ((pulse1+pulse2)<=wmax+tolerance2 || (pulse1+pulse2)>=wmin-tolerance)
		       {
			 if (pulse1+pulse2==wmax) wmaxcount++;
			 else if (pulse1+pulse2>wmax && (pulse1+pulse2)<=wmax+tolerance2) wmaxcount=1;
			      else if (pulse1+pulse2==wmin) wmincount=1;
				   else if (pulse1+pulse2<wmin && (pulse1+pulse2)>=wmin-tolerance2) wmincount=1;
			 wmax=((pulse1+pulse2)*((pulse1+pulse2)>wmax && (pulse1+pulse2)<=wmax+tolerance2))+(wmax*((pulse1+pulse2)<=wmax || (pulse1+pulse2)>wmax+tolerance2));
			 wmin=((pulse1+pulse2)*((pulse1+pulse2)<wmin && (pulse1+pulse2)>=wmin-tolerance2))+(wmin*((pulse1+pulse2)>=wmin || (pulse1+pulse2)<wmin+tolerance2));
		       }
		    else wavelen=(wmax+wmin)/4;
		  }
	   }
     }
  while (nobeep || noise);
  beeplength-=(pulse1+pulse2);
  if (!raw)
     {
       do {
	    beeplength+=(pulse1+pulse2);
	    pulse1=FindEdge1();
	    if (!blocktype) return 0;
	    if (pulse1<wavelen-tolerance || pulse1>wavelen+tolerance) break;
	    pulse2=FindEdge1();
	    if (!blocktype) return 0;
	    if (pulse2<wavelen-tolerance || pulse2>wavelen+tolerance) break;
	    if (pulse1 && pulse2)
	       {
		 if (pulse1+pulse2==wmax) wmaxcount++;
		 else if (pulse1+pulse2>wmax && (pulse1+pulse2)<=wmax+tolerance2) wmaxcount=1;
		      else if (pulse1+pulse2==wmin) wmincount++;
			   else if (pulse1+pulse2<wmin && (pulse1+pulse2)>=wmin-tolerance2) wmincount=1;
		 wmax=((pulse1+pulse2)*((pulse1+pulse2)>wmax && (pulse1+pulse2)<=wmax+tolerance2))+(wmax*((pulse1+pulse2)<=wmax || (pulse1+pulse2)>wmax+tolerance2));
		 wmin=((pulse1+pulse2)*((pulse1+pulse2)<wmin && (pulse1+pulse2)>=wmin-tolerance2))+(wmin*((pulse1+pulse2)>=wmin || (pulse1+pulse2)<wmin-tolerance2));
	       }
	  }
       while ((pulse1>wavelen-tolerance && pulse1<wavelen+tolerance) ||
	     (pulse2>wavelen-tolerance && pulse2<wavelen+tolerance));
     }
  if (pulse1>wavelen-tolerance && pulse1<wavelen+tolerance)
     {
       beeplength+=pulse1;
       if (pulse2>(wmax+wmin)/8) mspause+=((pulse2-((wmax+wmin)/8))/samprate)*1000;
     }
  else if (pulse1>(wmax+wmin)/8) mspause+=((pulse1-((wmax+wmin)/8))/samprate)*1000;
  if (wmaxcount>wmincount && wmincount<wmaxcount/2 || wmincount<15) wmin++;
  if (wmaxcount<wmincount && wmaxcount<wmincount/2 || wmaxcount<15) wmax--;
  msbeep=(beeplength*1000)/samprate;
  if (wmax+wmin) beepfreq=samprate/((float)(wmax+wmin)/2);
  else if (wavelen) beepfreq=samprate/wavelen;
  if (msbeep<30 || msbeep>100)
     {
       if (!tonesearch)
	  {
	    PopFilePointers();
	    lseek(inpfile.handle,filepos,SEEK_SET);
	    inpfile.bufpoint=0;
	    mspause+=(FindPause()/samprate)*1000;
	    WriteTZXPause(mspause);
	    endbeep=1;
	  }
     }
  else {
	 byte count;
	 word beeptstates;
	 if (beepfreq) beeptstates=(word)((float)(3500000/(2*beepfreq))+.5);
	 if (!tonesearch)
	    {
	      if (wavelen) WriteTZXTone(beeptstates,(samprate/(20*wavelen)));
	      pulses[1]=6;
	      for (count=0; count<6; count++)
		  { pulseseq[count]=bump[count]; }
	      WriteTZXPulseSequence(sizeof(bump));
	    }
       }
  return beepfreq;
} */

void DecryptToneSearch(void)
{
  tonesearch=1;
  pilotlength=BeepSearch(128);
  tonesearch=0;
  mspause+=(FindPause()/samprate)*1000;
}

word WaveSearch(void)
{
  byte maxsmp=(samprate/500)/2;
  word length=0;
  dword pulse,last,read;
  read=pulse=FindEdge1();
  if (!blocktype) return read;
  do {
       last=pulse;
       pulse=FindEdge1();
       if (!blocktype) return read;
       length=(((read+=pulse)/samprate)*1000);
     }
  while (abs(last-pulse)<maxsmp/4 || length<20);
  return length;
}

void WriteDecryptTone(void)
{
  if (hifi)
     {
       word tsplitsmp=127*bit1.samples+bit1.samples/1.375,
	    splitnum=beeplength/tsplitsmp+.5,
	    count;
       if (tzxhead[9]<0x0a)
	  for (count=0; count<splitnum; count++)
	      {
		WriteTZXTone(bit1.tstates,254);
		pulseseq[0]=bit1.tstates/1.375; pulseseq[1]=bit1.tstates/1.375;
		WriteTZXPulseSequence(2);
	      }
       else {
	      WriteTZXLoopStart(splitnum);
	      for (count=0; count<splitnum; count++)
		  {
		    WriteTZXTone(bit1.tstates,254);
		    pulseseq[0]=bit1.tstates/1.375; pulseseq[1]=bit1.tstates/1.375;
		    WriteTZXPulseSequence(2);
		  }
	      WriteTZXLoopEnd();
	    }
       WriteTZXTone(bit1.tstates,254);
       WriteTZXPause(mspause);
     }
  else {
	 WriteTZXTone(bit1.tstates,(beeplength/bit1.samples)*2);
	 WriteTZXPause(mspause);
       }
}

void WriteDecryptWaves(word wavnum)
{
  if (hifi)
     {
       if (tzxhead[9]<0x0a)
	  {
	    byte count1,count2;
	    for (count1=0; count1<wavnum; count1++)
		{
		  for (count2=0; count2<243; count2++)
		      {
			pulseseq[count2]=3500000/(samprate/((samprate/1000)-count2*.1345));
		      }
		  WriteTZXPulseSequence(243);
		}
	  }
       else {
	      byte count;
	      WriteTZXLoopStart(wavnum);
	      for (count=0; count<243; count++)
		  {
		    pulseseq[count]=3500000/(samprate/((samprate/1000)-count*.1345));
		  }
	      WriteTZXPulseSequence(243);
	      WriteTZXLoopEnd();
	    }
     }
  else {
	 double totsamp=0;
	 byte count;
	 for (count=0; count<243; count++) totsamp+=(samprate/1000)-count*.1345;
	 totsamp*=wavnum;
	 WriteTZXTone(bit1.tstates,(totsamp/bit1.samples)*2);
       }
}


void FindLoadAddress(word limit)
{
  byte push=0,found=0,opcode=0;
  word count;
  dword savepos=filepos;
  slockloadaddress=0;
  ResetTempFile();
  do
     {
       for (count=0; ((count<limit) && !found); count++)
	   {
	     if (!found) if ((GetData(&tempfile))==0x19) { found=1;break; }
	   }
       if (!found) Error(11);
       opcode=GetData(&tempfile);
       if (opcode==1)
	  {
	    slockloadaddress+=GetData(&tempfile);
	    slockloadaddress+=256*(GetData(&tempfile));
	  }
       if (opcode==0x11)
	  {
	    slockloadaddress+=GetData(&tempfile);
	    slockloadaddress+=256*(GetData(&tempfile));
	  }
       if (opcode==0xd5) push=1;
     }
  while (!push);
  filepos=savepos;
  lseek(inpfile.handle,filepos,SEEK_SET);
  inpfile.bufpoint=0;
  ResetTempFile();
}

void DecodeSpeedlockTable(word limit,byte offset)
{
  word count;
  dword savepos=filepos;
  if (zeta) { xorbyte=0x5a; addbyte=0; }
  ResetTempFile();
  if (limit>sizeof(slockdata)) Error(10);
  for (count=0; count<limit; count++)
      {
	slockdata[count+offset+tableoffset]=(byte)((GetData(&tempfile) ^ xorbyte)+addbyte);
	if (zeta)
	   {
	     slockdata[count+offset+tableoffset]^=0xff;
	     xorbyte+=0x35;
	   }
      }
  if (blocks==blockskip+2 && !subblocks)
     {
       for (count=0; count<limit; count++)
	   {
	     if (slocktype)
		{
		  if (slockdata[count]==0xdd && slockdata[count+1]==0x21 && count<0x11 && !tableoffset) { nxtloadpos=slockdata[count+2]+(256*slockdata[count+3]); }
		  if (slockdata[count]==0x11 && count<0x11 && !tableoffset) { expectedlength=1+slockdata[count+1]+(256*slockdata[count+2]); }
		  if ((slockdata[count]==0) && (slockdata[count+1]==0x80) && (slockdata[count+2]==0xff) && (slockdata[count+3]==0x02) && (slockdata[count+4]==0x10))
		     { tablepointer=tableoffset=count; break; }
		}
	     if (zeta)
		{
		  if ((slockdata[count]==0x10) && (slockdata[count+1]==0xff) && (slockdata[count+2]==0x02) && (slockdata[count+3]==0) && (slockdata[count+4]==0x80))
		     { tablepointer=tableoffset=count; break; }
		}
	   }
       if (!tableoffset)
	  {
	    byte rpage1=slockdata[0x27],rpage2=slockdata[0x65];
	    if (rpage1>=0x10 && rpage1<=0x17) tablepointer=tableoffset=0x23;
	    else if (rpage2>=0x10 && rpage2<=0x17) tablepointer=tableoffset=0x61;
	  }
     }
  loadertableadd=slockloadaddress+tablepointer;
  filepos=savepos;
  lseek(inpfile.handle,filepos,SEEK_SET);
  inpfile.bufpoint=0;
  ResetTempFile();
}

void GetLoadingParameters(word loaded)
{
  if (slocktype)
     {
       dword savepos=filepos;
       ResetTempFile();
       if ((nxtloadpos>slockloadaddress-0x319) && (nxtloadpos<slockloadaddress))
	  {
	    if (loaded==2)
	       {
		 GetData(&tempfile);
		 xorbyte=((GetData(&tempfile))^xorbyte)+addbyte;
	       }
	  }

       if ((nxtloadpos>=loadertableadd || nxtloadpos+loaded>loadertableadd) && ((slockloadaddress>0xc000 && nxtloadpos>0xc000) || (slockloadaddress<0xc000 && nxtloadpos<0xc000)))
	  {
	    if (loaded>128) { strcpy(errstring,"Missing mid-sync"); Error(8);}
	    else {
		   byte offset=nxtloadpos-loadertableadd;
		   DecodeSpeedlockTable(loaded,offset);
		 }
	  }
       expectedlength=slockdata[tablepointer+2]+256*(slockdata[tablepointer+3]);
       if (expectedlength)
	  {
	    expectedlength++;
	    SpeedEndBlock=0;
	    nxtloadpos=slockdata[tablepointer]+256*(slockdata[tablepointer+1]);
	    rampage=slockdata[tablepointer+4];
	  }
       else SpeedEndBlock=1;
       tablepointer+=5;
       filepos=savepos;
       lseek(inpfile.handle,filepos,SEEK_SET);
       inpfile.bufpoint=0;
     }
  if (soft)
     {
       if (add256)
	  {
	    ResetTempFile();
	    PushFilePointers();
	    opcode=MirrorByte(GetData(&tempfile));
	    if (nxtloadpos==0x5b00 && opcode!=0x3b)
	       {
		 nxtloadpos=0; return;
	       }
	    opcode=MirrorByte(GetData(&tempfile));
	    ResetTempFile();
	    PopFilePointers();
//          RecoverFilePointers();
	    add256=0;
	  }
       DataRead(bit0.samples,bit1.samples,4);
       WriteTZXPureData(8,4);
       PushFilePointers();
       ResetTempFile();
       expectedlength=MirrorByte(GetData(&tempfile));
       expectedlength+=(256*(MirrorByte(GetData(&tempfile))));
       nxtloadpos=MirrorByte(GetData(&tempfile));
       nxtloadpos+=(256*(MirrorByte(GetData(&tempfile))));
       if (nxtloadpos) nxtloadpos+=((256*(opcode==0x24))-(256*(opcode==0x25)));
       PopFilePointers();
       if (expectedlength<=5 || (nxtloadpos<23324 && nxtloadpos+expectedlength>23325)) add256=1;
       else add256=0;
     }
  if (zeta)
     {
       dword savepos=filepos;
       ResetTempFile();
       rampage=slockdata[tablepointer];
       if (rampage)
	  {
	    SpeedEndBlock=0;
	    expectedlength=1+slockdata[tablepointer+1]+256*(slockdata[tablepointer+2]);
	    nxtloadpos=slockdata[tablepointer+3]+256*(slockdata[tablepointer+4]);
	  }
       else SpeedEndBlock=1;
       tablepointer+=5;
       filepos=savepos;
       lseek(inpfile.handle,filepos,SEEK_SET);
       inpfile.bufpoint=0;
     }
}

byte MirrorByte(byte value)
{
  byte count;
  byte mirrored=0;
  for (count=0; count<8; count++)
      {
	mirrored>>=1;
	if (value&0x80) mirrored|=0x80;
	else mirrored&=0x7f;
	value<<=1;
     }
  return mirrored;
}

word ReadZetaLoadFirstBlock(void)
{
  word loaded;
  pilotlength=0;
  DataLoadOK=0;
  while (!DataLoadOK)
	{
	  PilotSearch(32);
	  if (!blocktype) return 0; if (!PilotLinkingOK) { pilotlength=0; return 0; }
	  mspause=0;
	  SyncSearch();
	  if (!blocktype) return 0; if (!SyncLoadOK) return 0;
	  bit0.freq=2044; bit1.freq=1022; SetTimings();
	  slockflagbits=4;
	  ReadSpeedlockFlag(bit0.samples,bit1.samples);
	  if (!blocktype) return 0;
	  bit0.freq=3100; bit1.freq=1550; SetTimings();
	  loaded=DataRead(bit0.samples,bit1.samples,expectedlength);
	  if (blocktype && blocks<=blockskip+1) mspause+=(FindPause()/samprate)*1000;
	}
  return loaded;
}

void RawRead(void)
{
   dword bytes;
   blocks++;
   raw=1;
//   header.type=0x0e;
   pilotlength=0;
   PushFilePointers();
   tonesearch=1;
   pilot.freq=BeepSearch(48);
   if (!pilot.freq) return;
   tonesearch=0;
   sync.freq=2450;
   SetTimings();
   PopFilePointers();
   lseek(inpfile.handle,filepos,SEEK_SET);
   inpfile.bufpoint=0;
   do {
	PilotSearch(256);
	if (!blocktype) return;
	SyncSearch();
	if (!blocktype) return;
      }
   while (!SyncLoadOK);
   if (!SyncLoadOK && lastpulse) sync.freq=samprate/(2*lastpulse); SetTimings();
   PushFilePointers();
   printf("Analysing pulses...            \xd");
   SearchDataFrequencies();
   if (!bit0.freq || !bit1.freq) return;
   PopFilePointers();
   lseek(inpfile.handle,filepos,SEEK_SET);
   inpfile.bufpoint=0;
   mspause=0;
   bytes=DataRead(bit0.samples,bit1.samples,0);
   if (blocktype) mspause+=(FindPause()/samprate)*1000;
   if (abs(pilot.freq-810)<60)
      {
	double ratio=(808/pilot.freq);
	pilot.freq=808; pilotlength/=ratio; mspause/=ratio;
	bit0.freq*=ratio; bit1.freq*=ratio;
      }
   if (abs(bit1.freq-1022)<=60)
      {
	bit0.freq=2*(bit1.freq=1022);
	raw=0;
      }
   else {
	  header.type=0x0e;
	  if (abs(bit1.freq-1225)<=70) { bit0.freq=2*(bit1.freq=1225); }
	  else if (soft || abs(bit1.freq-1300)<=50) { bit0.freq=2*(bit1.freq=1300); }
	     //else if (abs(bit1.freq-1400)<50) { bit0.freq=2*(bit1.freq=1400); SetTimings(); }
	       else if (slocktype || (!alk && abs(bit1.freq-1480)<55)) { bit0.freq=2*(bit1.freq=1500); }
		    else if (alk || (!slocktype && abs(bit1.freq-1550)<15)) { bit0.freq=2*(bit1.freq=1550); }
	}
   SetTimings();
   if (bytes==19)
      {
	byte count;
	dword savefilepos=filepos;
	ResetTempFile();
	if (GetData(&tempfile)==0)
	   {
	     header.type=GetData(&tempfile);
	     for (count=0; count<10; count++)
		 {
		   header.name[count]=GetData(&tempfile);
		   if (header.name[count]<' ') header.name[count]='*';
		 }
	     header.name[10]=0;
	     header.length=(byte)(GetData(&tempfile));
	     header.length+=(word)(256*GetData(&tempfile));
	     header.start=(byte)(GetData(&tempfile));
	     header.start+=(word)(256*GetData(&tempfile));
	     expectedlength=(header.length)+2;
	     if (header.type==0) nxtloadpos=header.start=23755;
	     else nxtloadpos=header.start;
	   }
	filepos=savefilepos;
      }
   else {
	  if (header.type>3) header.start=0;
	  if (header.type!=0xe) header.type=0xf;
	}

   PrintBlockInfo(header.start,bytes);
   if (pilot.freq==808 && sync.freq==2450 && bit0.freq==2044 && bit1.freq==1022) WriteTZXNormalBlock(bytes);
   else WriteTZXTurboBlock(pilotlength,8,bytes);
   raw=0;
}

void SearchDataFrequencies(void)
{
  dword dwpulse,count,pulsecount=0;
//  double tolerance=2; //((125-(samprate/282.22))/samprate)*1000;
  double pmax=0,pmin=0,totmin=0,totmax=0,startmax,startmin,min,max;
  byte pulse;
  wmincount=wmaxcount=0;
/*  do {
       dwpulse=FindEdge2();
       if (!blocktype) break;
       if (dwpulse<samprate/808 && dwpulse)
	  {
	    pulse=dwpulse;
	    PutData(&tempfile,pulse,1);
	    pulsecount++;
	  }
     }
  while (dwpulse<pilot.samples && dwpulse);
  ResetTempFile();
  for (count=0; count<2; count++) GetData(&tempfile); */
//  FindEdge2();
//  FindEdge2();
//  for (count=0; count<2; count++)
//      {
	pmax=pmin=FindEdge2(); //GetData(&tempfile);
	if (pmax*2-pilot.samples+.5>-3)
	   {
	     startmin=totmin=pmin=pmax*.7;
	     pmax--;
	     startmax=totmax=pmax;
	     wmincount++;
	     wmaxcount++;
	   }
	else {
	       startmax=totmax=pmax=pmin*1.8;
	       pmin++;
	       startmin=totmin=pmin;
	       wmaxcount++;
	       wmincount++;
	     }
/*  wmaxcount=wmincount=0;
  if (pulsecount<10) { bit0.freq=bit1.freq=0; return; }
  for (count=0; count<pulsecount-5; count++) */
	do {
	     pulse=FindEdge2(); //GetData(&tempfile);
	     pulsecount++;
	     if (pulse)
		{
		  if (abs(pulse-pmax)<2) { wmaxcount++; totmax+=pulse; }
		  else if (abs(pulse-pmin)<2) { wmincount++; totmin+=pulse; }
		  //if (pulse==pmax || (pulse>pmax && pulse<=pmax*1.5 && abs(pmin*2-pulse)<2)) { wmaxcount++; totmax+=pulse; }// && pulse<=pmax*1.5) wmaxcount++;
		  //else if (pulse>pmax && pulse<=pmax*1.5) wmaxcount=1;
		       //else if (pulse==pmin || (pulse<pmin && pulse>=pmin*.5 && abs(pmax/2-pulse)<2)) { wmincount++; totmin+=pulse; }//&& pulse>=pmin*.5) wmincount++;
			    //else if (pulse<pmin && pulse>=pmin*.5) wmincount=1;
		  pmax=(pulse*(pulse>pmax && (pulse<=pmin*2.5 || pulse<=pmax*1.5)))+(pmax*(pulse<=pmax));
		  pmin=(pulse*(pulse<pmin && (pulse>=pmax/3 || pulse>=pmin*.5)))+(pmin*(pulse>=pmin));
//                diff=abs(pmax-2*pmin);
		}
	   }
	while (pulsecount<21 && pulse<pmax*1.5 && pulse>pmin*.5);
//      totmin+=pmin;
//      totmax+=pmax;
//      }
//  if (wmaxcount>wmincount && wmincount<wmaxcount/2 || wmincount<15) pmin+=.25;
//  if (wmaxcount<wmincount && wmaxcount<wmincount/2 || wmaxcount<15) pmax-=.75;
//  pmax+=((pmin*2+pmax)/3-middle/pulsecount);
//  pmin=(3*pmin+6*pmax)/15; pmax=2*pmin;  // pmax++; pmin++;
//  pmax+=1.2-abs(diff); pmin+=.4-abs(diff/2);
//  ratio=(double)max(wmaxcount,wmincount)/min(wmaxcount,wmincount);
/*  if (pmax-start*2>pmin/2) { totmax+=start; totmin-=start; wmincount--; }
  else { totmax-=start; wmaxcount--; } */
  min=totmin/wmincount; max=totmax/wmaxcount;
  if (abs(pmin*2-pmax)>pmin/2) { pmin=min; pmax=max; }
  else {
	 if ((wmincount==1 && wmaxcount>1) || abs(pmin-startmin)<.01) pmin=max/2;
	    else if ((wmaxcount==1 && wmincount>1) || abs(pmax-startmax)<0.1) pmax=min*2;
	 pmin=(pmin+pmax)/3; pmax=2*pmin;
       }
//  pmax+=(pmax*(2*ratio/3)/100);
//  pmin-=(pmin*(ratio/3)/100);
/*  pmin=(pmin+pmax)/3; pmax=pmin*2; //pmin=diff/4; pmax-=diff/2;*/
  if (pmin && pmax) bit0.freq=(samprate/pmin); bit1.freq=(samprate/pmax); SetTimings();
  ResetTempFile();
}

/***************************************************************************/

void PrintBlockInfo(word loadadd,dword biglen)
{
  byte type=header.type;
  word len=biglen;
  dword lpause=mspause;
  byte part=type&0x0f;
  if (!subblocks && part!=0x0a && part!=0x0b && part!=0x0c) printf("Block %2d => ",blocks);
  else printf("            ");
  if (!hex) {
	      switch (type)
		     {
		       case    0 : printf("Program: "); break;
		       case    1 : printf("N.Array: "); break;
		       case    2 : printf("C.Array: "); break;
		       case    3 : printf("  Bytes: "); break;
		       case    8 : printf(" -->Sub-block %2u    - ",subblocks);
				   if (slocktype>4 || soft || zeta) printf("Start=%5u, ",loadadd);
				   printf("Length=%5u",len);
				   if (slocktype>4 || zeta) printf(", RAM page=%u",rampage);
				   printf("\n"); break;
		       case    9 : printf(" -->Sub-block %.3u   - Start=%5u, Length=%3u, Pause=%ums.\n",subblocks,nxtloadpos,len,lpause); break;
		       case 0x0a : printf("Decryption beeps    - Total=%3u, ",beeps);
				   printf("Pause=%lums.\n",lpause); break;
		       case 0x0b : printf("Decryption waves    - Total=%3u, Pause=%lums.\n",len,lpause); break;
		       case 0x0c : printf("Decryption tone     - Length=%ums., Pause=%lums.\n",len,lpause); break;
		       case 0x0e :
		       case 0x0f : if (!raw) printf("------------------- - ");
				   else printf("F: %2X - Speed: %3u% - ",flag,(dword)bit1.freq*100/1022);
				   if (type==0x0f && header.start) printf("Start=%5u, ",loadadd);
				   else {
					  printf("Chk=");
					  if (alk) printf("???");
					  else if (!DataLoadOK) printf("ERR");
					       else printf("OK!");
					  printf(" (%.2X), ",chksum);
					}
				   printf("Length=%5u, Pause=%lums.\n",len,lpause);
				   break;
		       case 0x11 :
		       case 0x12 :
		       case 0x13 :
		       case 0x21 :
		       case 0x31 :
		       case 0x41 : printf("Speedlock %u Block %u - ",(type&0xf0)/0x10,type&0x0f);
				   if (type==0x12 && (0x10000-len<32768 || len==0x1b00)) printf("Start=16384, ");
				   printf("Length=%5u, Pause=%lums.\n",len,lpause); break;
		       case 0x51 :
		       case 0x61 :
		       case 0x71 : printf("Speedlock %u Block 1 - Start=%5u, Length=%5u, Pause=%lums.\n",(type&0xf0)/0x10,loadadd,len,lpause); break;
		       case 0x22 :
		       case 0x32 :
		       case 0x42 :
		       case 0x52 :
		       case 0x62 :
		       case 0x72 : printf("Speedlock %u Block 2 - Multiple sub-blocks sequence starting\n",(type&0xf0)/0x10); break;
		       case 0x81 : printf("Alkatraz Block 1    - Start=%5u, Length=%5u, Pause=%lums.\n",alkloadpos,len,lpause); break;
		       case 0x82 : printf("Alkatraz Block 2    - Length=%5lu, Pause=%ums.\n",biglen,lpause); break;
		       case 0x91 :
		       case 0x92 :
		       case 0x93 : printf("Bleepload Group %2u  - Multiple sub-blocks sequence starting\n",type-0x90); break;
		       case 0x9f : printf("Bleepload Group End - Total Length=%lu\n",biglen); break;
		       case 0xa1 : printf("Softlock Block      - Multiple sub-blocks sequence starting\n"); break;
		       case 0xb1 : printf("ZetaLoad Block 1    - Start=%5u, Length=%5u, Pause=%lums.\n",loadadd,len,lpause); break;
		       case 0xb2 : printf("ZetaLoad Block 2    - Multiple sub-blocks sequence starting\n"); break;
		       case 0xff : printf("Block End           - Total Length=%lu, Pause=%ums.\n",biglen,lpause); break;
		   }
	      if (type<4) printf("%s - Header: Length=%2u, Pause=%lums.\n",header.name,17,lpause);
	    }
  else {
	 switch (header.type)
		{
		  case    0 : printf("Program: "); break;
		  case    1 : printf("N.Array: "); break;
		  case    2 : printf("C.Array: "); break;
		  case    3 : printf("  Bytes: "); break;
		  case    8 : printf(" -->Sub-block %2u    - ",subblocks);
			      if (slocktype>4 || soft || zeta) printf("Start=%.4X, ",loadadd);
			      printf("Length=%.4X",len);
			      if (slocktype>4 || zeta) printf(", RAM page=%X",rampage);
			      printf("\n"); break;
		  case    9 : printf(" -->Sub-block %.3u   - Start=%.4X, Length=%.3X, Pause=%ums.\n",subblocks,nxtloadpos,len,lpause); break;
		  case 0x0a : printf("Decryption beeps    - Total=%2X, ",beeps);
			      printf("Pause=%lums.\n",lpause); break;
		  case 0x0b : printf("Decryption waves    - Total=%2X, Pause=%lums.\n",len,lpause); break;
		  case 0x0c : printf("Decryption tone     - Length=%ums., Pause=%lums.\n",len,lpause); break;
		  case 0x0e :
		  case 0x0f : if (!raw) printf("------------------- - ");
			      else printf("F: %2X - Speed: %3u% - ",flag,(dword)bit1.freq*100/1022);
			      if (type==0x0f && header.start) printf("Start=%.4X, ",loadadd);
			      else {
				     printf("Chk=");
				     if (alk) printf("???");
				     else if (!DataLoadOK) printf("ERR");
					  else printf("OK!");
				     printf(" (%.2X), ",chksum);
				   }
			      printf("Length=%.4X, Pause=%lums.\n",len,lpause);
			      break;
		  case 0x11 :
		  case 0x12 :
		  case 0x13 :
		  case 0x21 :
		  case 0x31 :
		  case 0x41 : printf("Speedlock %u Block %u - ",(type&0xf0)/0x10,type&0x0f);
			      if (type==0x12 && (0x10000-len<32768 || len==0x1b00)) printf("Start=4000, ");
			      printf("Length=%.4X, Pause=%lums.\n",len,lpause); break;
		  case 0x51 :
		  case 0x61 :
		  case 0x71 : printf("Speedlock %u Block 1 - Start=%.4X, Length=%.4X, Pause=%lums.\n",(type&0xf0)/0x10,loadadd,len,lpause); break;
		  case 0x22 :
		  case 0x32 :
		  case 0x42 :
		  case 0x52 :
		  case 0x62 :
		  case 0x72 : printf("Speedlock %u Block 2 - Multiple sub-blocks sequence starting\n",(type&0xf0)/0x10); break;
		  case 0x81 : printf("Alkatraz Block 1    - Start=%.4X, Length=%.4X, Pause=%lums.\n",alkloadpos,len,lpause); break;
		  case 0x82 : printf("Alkatraz Block 2    - Length=%.5lX, Pause=%lums.\n",biglen,lpause); break;
		  case 0x91 :
		  case 0x92 :
		  case 0x93 : printf("Bleepload Group %2u  - Multiple sub-blocks sequence starting\n",type-0x90); break;
		  case 0x9f : printf("Bleepload Group End - Total Length=%lX\n",biglen); break;
		  case 0xa1 : printf("Softlock Block      - Multiple sub-blocks sequence starting\n"); break;
		  case 0xb1 : printf("ZetaLoad Block 1    - Start=%.4X, Length=%.4X, Pause=%lums.\n",loadadd,len,lpause); break;
		  case 0xb2 : printf("ZetaLoad Block 2    - Multiple sub-blocks sequence starting\n"); break;
		  case 0xff : printf("Block End           - Total Length=%lX, Pause=%ums.\n",biglen,lpause); break;
		}
	 if (type<4) printf("%s - Header: Length=%.2X, Pause=%lums.\n",header.name,17,lpause);
       }
}

/***************************************************************************/
void WriteTZXHeader(void)
{
  byte count;
  for (count=0; count<sizeof(tzxhead); count++) PutData(&outfile,tzxhead[count],1);
}

void WriteTZXNormalBlock(word datalen)
{
  word count;
  PushFilePointers();
  stdspeed[1]=Lo(mspause); stdspeed[2]=Hi(mspause);
  stdspeed[3]=Lo(datalen); stdspeed[4]=Hi(datalen);
  for (count=0; count<sizeof(stdspeed); count++) PutData(&outfile,stdspeed[count],1);
  ResetTempFile();
  for (count=0; count<datalen; count++) PutData(&outfile,GetData(&tempfile),1);
  PopFilePointers();
  lseek(inpfile.handle,filepos,SEEK_SET);
  inpfile.bufpoint=0;
  ResetTempFile();
}

void WriteTZXTurboBlock(dword lpilot,byte lastbyte,dword datalen)
{
  dword count;
  word pilotpulses=2*lpilot/pilot.samples;
  PushFilePointers();
  turboblock[1]=Lo(pilot.tstates); turboblock[2]=Hi(pilot.tstates);
  turboblock[3]=Lo(sync.tstates); turboblock[4]=Hi(sync.tstates);
  turboblock[5]=Lo(sync.tstates); turboblock[6]=Hi(sync.tstates);
  turboblock[7]=Lo(bit0.tstates); turboblock[8]=Hi(bit0.tstates);
  turboblock[9]=Lo(bit1.tstates); turboblock[10]=Hi(bit1.tstates);
  turboblock[11]=Lo(pilotpulses); turboblock[12]=Hi(pilotpulses);
  turboblock[13]=lastbyte;
  turboblock[14]=Lo(mspause); turboblock[15]=Hi(mspause);
  turboblock[16]=Lo(datalen); turboblock[17]=Hi(datalen); turboblock[18]=Ehi(datalen);
  for (count=0; count<sizeof(turboblock); count++) PutData(&outfile,turboblock[count],1);
  ResetTempFile();
  for (count=0; count<datalen; count++) PutData(&outfile,GetData(&tempfile),1);
  PopFilePointers();
  lseek(inpfile.handle,filepos,SEEK_SET);
  inpfile.bufpoint=0;
  ResetTempFile();
}

void WriteTZXTone(word plen,word tlen)
{
  byte count;
  puretone[1]=Lo(plen); puretone[2]=Hi(plen);
  puretone[3]=Lo(tlen); puretone[4]=Hi(tlen);
  for (count=0; count<sizeof(puretone); count++) PutData(&outfile,puretone[count],1);
}

void WriteTZXPulseSequence(byte pulsenum)
{
  byte count;
  pulses[1]=pulsenum;
  for (count=0; count<sizeof(pulses); count++) PutData(&outfile,pulses[count],1);
  for (count=0; count<pulsenum; count++)
      {
	PutData(&outfile,Lo(pulseseq[count]),1);
	PutData(&outfile,Hi(pulseseq[count]),1);
      }
}

void WriteTZXPureData(byte lastbyte,dword datalen)
{
  dword count;
  puredata[1]=Lo(bit0.tstates); puredata[2]=Hi(bit0.tstates);
  puredata[3]=Lo(bit1.tstates); puredata[4]=Hi(bit1.tstates);
  puredata[5]=lastbyte;
  if (!saveslockflag) { puredata[6]=Lo(mspause); puredata[7]=Hi(mspause); }
  else puredata[6]=puredata[7]=0;
  puredata[8]=Lo(datalen); puredata[9]=Hi(datalen); puredata[10]=Ehi(datalen);
  for (count=0; count<sizeof(puredata); count++) PutData(&outfile,puredata[count],1);
  if (saveslockflag) PutData(&outfile,slockflag,1);
  else if (savemidsync)
	  { for (count=0; count<datalen; count++) PutData(&outfile,midsync[count],1); }
       else {
	      PushFilePointers();
	      ResetTempFile();
	      for (count=0; count<datalen; count++)
		  { PutData(&outfile,GetData(&tempfile),1); }
	      PopFilePointers();
	      lseek(inpfile.handle,filepos,SEEK_SET);
	      inpfile.bufpoint=0;
	    }
  ResetTempFile();
}

void WriteTZXPause(word pauselen)
{
  byte count;
  pause[1]=Lo(pauselen); pause[2]=Hi(pauselen);
  for (count=0; count<sizeof(pause); count++) PutData(&outfile,pause[count],1);
}

void WriteTZXGroupStart(void)
{
  byte count;
  byte ldrtype=((header.type)&0xf0)/0x10,block=(header.type)&0x0f;
  char bnum[2];
  itoa(block,bnum,10);
  if (!ldrtype)
     {
       switch (block)
	      {
		case 0x0a : strcpy(groupname,"Decryption Beeps\0"); break;
		case 0x0b : strcpy(groupname,"Decryption Waves\0"); break;
		case 0x0c : strcpy(groupname,"Decryption Tone\0"); break;
	      }
     }
  if (ldrtype>=1 && ldrtype<=7)
     {
       char stype[2];
       itoa(ldrtype,stype,10);
       strcpy(groupname,"Speedlock ");
       strcat(groupname,stype);
     }
  if (ldrtype==9)
     { strcpy(groupname,"Bleepload"); }
  if (ldrtype==0x0b)
     { strcpy(groupname,"ZetaLoad"); }
  if (ldrtype)
     {
       strcat(groupname," Block ");
       strcat(groupname,bnum);
       strcat(groupname,"\0");
     }
  groupstart[1]=strlen(groupname);
  for (count=0; count<sizeof(groupstart); count++) PutData(&outfile,groupstart[count],1);
  for (count=0; count<groupstart[1]; count++) PutData(&outfile,groupname[count],1);
}

void WriteTZXGroupEnd(void)
{ PutData(&outfile,groupend[0],1); }

void WriteTZXLoopStart(word times)
{
  byte count;
  loopstart[1]=Lo(times); loopstart[2]=Hi(times);
  for (count=0; count<sizeof(loopstart); count++) PutData(&outfile,loopstart[count],1);
}

void WriteTZXLoopEnd(void)
{ PutData(&outfile,loopend[0],1); }

void WriteTZXText(void)
{
  byte count;
  text[1]=strlen(descr);
  for (count=0; count<sizeof(text); count++) PutData(&outfile,text[count],1);
  for (count=0; count<text[1]; count++) PutData(&outfile,descr[count],1);
}

/***************************************************************************/

void main(int argc, char **argv)
{
  char *fnampoint;

  ARGC=argc; ARGV=argv;

  spdchk=1;
  CheckSyntax();
  strcpy(tempfile.name,"mtzx000.tmp\0");
  stack1.busy=stack2.busy=stack3.busy=0;
  inpfile.handle=OpenFile(inpfile.name);
  inpfile.length=lseek(inpfile.handle,0,SEEK_END);
  lseek(inpfile.handle,0,SEEK_SET);
  outfile.handle=CreateFile(outfile.name);
  tempfile.handle=CreateFile(tempfile.name);
  if (loggen)
     {
       logfile=fopen("MAKETZX.LOG","w+");
       //getdate(&curdate); gettime(&curtime);
       //fprintf(logfile,"MAKETZX v%s - Log file generated on %d-%d-%d at %d:%02d\n\n",version,curdate.da_mon,curdate.da_day,curdate.da_year,curtime.ti_hour,curtime.ti_min);
       fprintf(logfile,"Input file name: %s\nOutput file name: %s\n\n",inpfile.name,outfile.name);

     }
  pilot.freq=808; sync.freq=2450; bit0.freq=2044; bit1.freq=1022;
  fnampoint=strrchr(inpfile.name,'\\');
  if (fnampoint==NULL) fnampoint=strupr(inpfile.name);
  else fnampoint=strupr(fnampoint)+1;
  printf("Processing file: %s\n",fnampoint);
  CheckVOCHeader();
  WriteTZXHeader();
  strcpy(descr,"Created with Ramsoft MAKETZX");
  WriteTZXText();
  if (samprate<11025) printf("***WARNING*** Sampling rate is too low. Raw mode may not work!\n\n");
  printf("þ Using TZX format revision %s\n",tzxver);
  printf("þ Printing file info in ");
  if (hex) printf("hex");
  else printf("decimal");
  printf(" format\n");
  printf("þ Sampling rate: %.0f Hz\n",samprate);
  if (normal)
     {
	printf("þ Operating in NORMAL mode\n\n");
       do {
	    word bytes;
	    blocks++;
	    bytes=ReadNormalBlock();
	    if (DataLoadOK)
	       {
		 PrintBlockInfo(nxtloadpos,bytes-2);
		 WriteTZXNormalBlock(bytes);
	       }
	    else if (blocktype) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
	  }
       while (blocktype);
     }
  if (soft)
     {
       dword grandtotal=0;
       word bytes;
       printf("þ Operating in SOFTLOCK mode\n\n");
       if (!blockskip) blockskip=2;
       for (blocks=1; blocks<blockskip+1; blocks++)
	   {
	     bytes=ReadNormalBlock();
	     if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
	     else PrintBlockInfo(nxtloadpos,bytes-2);
	     WriteTZXNormalBlock(bytes);
	   }
       header.type=0xa1;
       PrintBlockInfo(0,0);
       subblocks=1;
       nxtloadpos=0x4000; expectedlength=0x1c02;
       grandtotal+=(bytes=ReadSoftlockFirstBlock());
       if (checksum) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8);}
       header.type=8;
       PrintBlockInfo(nxtloadpos,bytes-2);
       WriteTZXTurboBlock(pilotlength,8,bytes);
       do {
	    GetLoadingParameters(0);
	    if (nxtloadpos)
	       {
		 ResetTempFile();
		 grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,expectedlength+1));
		 if (checksum) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
		 subblocks++;
		 PrintBlockInfo(nxtloadpos,bytes-1);
		 WriteTZXPureData(8,bytes);
	       }
	    else {
		   header.type=255;
		   subblocks=0;
		   ResetTempFile();
		   grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,1));
		   mspause+=(FindPause()/samprate)*1000;
		   PrintBlockInfo(0,grandtotal);
		   WriteTZXPureData(8,1);
		 }
	  }
       while (nxtloadpos);
     }
  if (bleep)
     {
       dword grandtotal=0;
       byte groupcount;
       word bytes;
       printf("þ Operating in BLEEPLOAD mode\n\n");
       if (!blockskip) blockskip=4;
       for (blocks=1; blocks<blockskip+1; blocks++)
	   {
	     bytes=ReadNormalBlock();
	     if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
	     else PrintBlockInfo(nxtloadpos,bytes-2);
	     WriteTZXNormalBlock(bytes);
	   }
       StoreBleeploadCode(bytes-2);
       header.type=0x91;
       PrintBlockInfo(0,0);
       WriteTZXGroupStart();
       for (subblocks=1; subblocks<0x31; subblocks++)
	   {
	     header.type=9;
	     grandtotal+=(bytes=ReadBleeploadBlock());
	     if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
	     GetBleeploadInfo(bytes);
	     if (bleepblock[0x101]!=subblocks-1) Error(12);
	     PrintBlockInfo(nxtloadpos,bytes-2);
	     WriteTZXTurboBlock(pilotlength,8,bytes);
	   }
       header.type=0x9f;
       subblocks=0;
       PrintBlockInfo(0,grandtotal);
       WriteTZXGroupEnd();
       blocks++;
       header.type=0x92;
       PrintBlockInfo(0,0);
       WriteTZXGroupStart();
       ReadBleeploadGroup();
       if (SearchBleeploadGroup3())
	  {
	    WriteTZXGroupEnd();
	    blocks++;
	    header.type=0x93;
	    PrintBlockInfo(0,0);
	    WriteTZXGroupStart();
	    ReadBleeploadGroup();
	    WriteTZXGroupEnd();
	  }
       else WriteTZXGroupEnd();
     }
  if (alk)
     {
       dword bytes;
       printf("þ Operating in ALKATRAZ mode\n\n");
       if (!blockskip) blockskip=2;
       for (blocks=1; blocks<blockskip+1; blocks++)
	   {
	     bytes=ReadNormalBlock();
	     if (DataLoadOK) PrintBlockInfo(nxtloadpos,bytes-2);
	     WriteTZXNormalBlock(bytes);
	   }
       alkloadpos=FindAlkatrazLoader1(bytes);
       do
	   {
	     header.type=(blocks-2+0x80);
	     bytes=ReadAlkatrazBlock();
	     PrintBlockInfo(alkloadpos,bytes-3);
	     WriteTZXTurboBlock(pilotlength,8,bytes);
	     ResetTempFile();
	     blocks++;
	   }
       while (blocks<blockskip+3);
     }
  if (slocktype)
     {
       printf("þ Operating in SPEEDLOCK %u mode\n\n",slocktype);
       switch (slocktype)
	      {
		case 1 : {
			   word bytes;
			   if (!blockskip) blockskip=2;
			   for (blocks=1; blocks<blockskip+1; blocks++)
			       {
				 bytes=ReadNormalBlock();
				 if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
				 PrintBlockInfo(nxtloadpos,bytes-2);
				 WriteTZXNormalBlock(bytes);
			       }
			   bit0.freq=3100;
			   bit1.freq=1550;
			   SetTimings();
			   header.type=0x11;
			   WriteTZXGroupStart();
			   bytes=ReadSpeedlock13FirstBlock();
			   if (!DataLoadOK) { strcpy(errstring,"Bad pilot detection or data error"); Error(8); }
			   PrintBlockInfo(0,bytes-1);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   WriteTZXGroupEnd();
			   do {
				blocks++;
				header.type++;
				WriteTZXGroupStart();
				bytes=ReadSpeedlock13FirstBlock();
				if (!ClickingPilotOK) break;
				if (!DataLoadOK) { strcpy(errstring,"Bad pilot detection or data error"); Error(8); };
				PrintBlockInfo(nxtloadpos,bytes-1);
				saveslockflag=1;
				WriteTZXPureData(slockflagbits+1,1);
				saveslockflag=0;
				WriteTZXPureData(8,bytes);
				mspause+=(FindPause()/samprate)*1000;
				WriteTZXPause(mspause);
				WriteTZXGroupEnd();
			      }
			   while (ClickingPilotOK && blocktype);
			 } break;
		case 2 : {
			   dword grandtotal=0;
			   byte pnum;
			   word bytes;
			   if (!blockskip) blockskip=4;
			   for (blocks=1; blocks<blockskip+1; blocks++)
			       {
				 bytes=ReadNormalBlock();
				 if (DataLoadOK) PrintBlockInfo(nxtloadpos,bytes-2);
				 else { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
				 WriteTZXNormalBlock(bytes);
			       }
			   bit0.freq=3100;
			   bit1.freq=1550;
			   SetTimings();
			   header.type=0x21;
			   WriteTZXGroupStart();
			   bytes=ReadSpeedlock13FirstBlock();
			   if (!DataLoadOK) { strcpy(errstring,"Bad pilot detection or data error"); Error(8); }
			   PrintBlockInfo(0,bytes-1);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   WriteTZXGroupEnd();
			   blocks++;
			   header.type=0x22;
			   PrintBlockInfo(0,0);
			   WriteTZXGroupStart();
			   nxtloadpos=expectedlength=rampage=0;
			   header.type=0x8;
			   subblocks=1;
			   grandtotal+=(bytes=ReadSpeedlock13FirstBlock());
			   PrintBlockInfo(nxtloadpos,bytes);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   if (!MidSyncSearch()) { strcpy(errstring,"Bad sync pulse detection or missing mid-sync"); Error(8); }
			   bit0.freq=3100; bit1.freq=1550;
			   SetTimings();
			   do {
				subblocks++;
				grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,expectedlength));
				PrintBlockInfo(nxtloadpos,bytes);
				WriteTZXPureData(8,bytes);
				SpeedEndBlock=!MidSyncSearch();
				bit0.freq=3100; bit1.freq=1550;
				SetTimings();
			      }
			   while (!SpeedEndBlock);
			   mspause=0;
			   if (checksum)
			      {
				if (bytes-probend>2) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
				else bytes=probend;
			      }
			   mspause+=(FindPause()/samprate)*1000;
			   WriteTZXPause(mspause);
			   header.type=0xff;
			   subblocks=0;
			   PrintBlockInfo(0,grandtotal);
			   WriteTZXGroupEnd();
			 } break;
		case 3 : {
			   dword grandtotal=0;
			   byte pnum;
			   word bytes;
			   if (!blockskip) blockskip=6;
			   for (blocks=1; blocks<blockskip+1; blocks++)
			       {
				 bytes=ReadNormalBlock();
				 if (DataLoadOK) PrintBlockInfo(nxtloadpos,bytes-2);
				 else { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
				 WriteTZXNormalBlock(bytes);
			       }
			   header.type=0x0a;
			   WriteTZXGroupStart();
			   endbeep=0;
			   printf("Counting beeps...              \xd");
			   do {
				BeepSearch(15);
				beeps++;
			      }
			   while (!endbeep);
			   beeps--;
			   PrintBlockInfo(0,0);
			   WriteTZXGroupEnd();
			   bit0.freq=3100;
			   bit1.freq=1550;
			   SetTimings();
			   header.type=0x31;
			   WriteTZXGroupStart();
			   bytes=ReadSpeedlock13FirstBlock();
			   if (!DataLoadOK) { strcpy(errstring,"Bad pilot detection or data error"); Error(8); }
			   PrintBlockInfo(0,bytes-1);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   WriteTZXGroupEnd();
			   blocks++;
			   header.type=0x32;
			   WriteTZXGroupStart();
			   PrintBlockInfo(0,0);
			   nxtloadpos=expectedlength=rampage=0;
			   header.type=0x8;
			   subblocks=1;
			   grandtotal+=(bytes=ReadSpeedlock13FirstBlock());
			   PrintBlockInfo(nxtloadpos,bytes);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   if (!MidSyncSearch()) { strcpy(errstring,"Bad sync pulse detection or missing mid-sync"); Error(8); }
			   bit0.freq=3100; bit1.freq=1550;
			   SetTimings();
			   do {
				subblocks++;
				grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,expectedlength));
				PrintBlockInfo(nxtloadpos,bytes);
				WriteTZXPureData(8,bytes);
				SpeedEndBlock=!MidSyncSearch();
				bit0.freq=3100; bit1.freq=1550;
				SetTimings();
			      }
			   while (!SpeedEndBlock);
			   mspause=0;
			   if (checksum)
			      {
				if (bytes-probend>2) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
				else bytes=probend;
			      }
			   mspause+=(FindPause()/samprate)*1000;
			   WriteTZXPause(mspause);
			   header.type=0xff;
			   subblocks=0;
			   PrintBlockInfo(0,grandtotal);
			   WriteTZXGroupEnd();
			 } break;
		case 4 : {
			   dword grandtotal=0;
			   byte pnum;
			   word bytes;
			   if (!blockskip) blockskip=4;
			   for (blocks=1; blocks<blockskip+1; blocks++)
			       {
				 bytes=ReadNormalBlock();
				 if (DataLoadOK) PrintBlockInfo(nxtloadpos,bytes-2);
				 else { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
				 WriteTZXNormalBlock(bytes);
			       }
			   header.type=0x0a;
			   WriteTZXGroupStart();
			   endbeep=0;
			   printf("Counting beeps...              \xd");
			   do {
				BeepSearch(15);
				beeps++;
			      }
			   while (!endbeep);
			   beeps--;
			   WriteTZXGroupEnd();
			   PrintBlockInfo(0,0);
			   bit0.freq=3100;
			   bit1.freq=1550;
			   SetTimings();
			   header.type=0x41;
			   WriteTZXGroupStart();
			   bytes=ReadSpeedlock47FirstBlock();
			   if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
			   PrintBlockInfo(0,bytes-1);
			   WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
			   blocks++;
			   pulseseq[0]=sync.tstates; pulseseq[1]=sync.tstates;
			   for (pnum=0; pnum<7; pnum++)
			       {
				 word plen=3500000/(2*compositesync[pnum]);
				 pulseseq[2*pnum+2]=plen;
				 pulseseq[2*pnum+3]=plen;
			       }
			   WriteTZXPulseSequence(16);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   WriteTZXGroupEnd();
			   header.type=0x42;
			   PrintBlockInfo(0,0);
			   WriteTZXGroupStart();
			   nxtloadpos=expectedlength=rampage=0;
			   header.type=0x8;
			   subblocks=1;
			   grandtotal+=(bytes=ReadSpeedlock47FirstBlock());
			   PrintBlockInfo(nxtloadpos,bytes);
			   WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
			   pulseseq[0]=sync.tstates; pulseseq[1]=sync.tstates;
			   for (pnum=0; pnum<7; pnum++)
			       {
				 word plen=3500000/(2*compositesync[pnum]);
				 pulseseq[2*pnum+2]=plen;
				 pulseseq[2*pnum+3]=plen;
			       }
			   WriteTZXPulseSequence(16);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   if (!MidSyncSearch()) { strcpy(errstring,"Bad sync pulse detection or missing mid-sync"); Error(8); }
			   bit0.freq=3100; bit1.freq=1550;
			   SetTimings();
			   do {
				subblocks++;
				grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,expectedlength));
				PrintBlockInfo(nxtloadpos,bytes);
				WriteTZXPureData(8,bytes);
				SpeedEndBlock=!MidSyncSearch();
				bit0.freq=3100; bit1.freq=1550;
				SetTimings();
			      }
			   while (!SpeedEndBlock);
			   mspause=0;
			   if (checksum)
			      {
				if (bytes-probend>2) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
				else bytes=probend;
			      }
			   mspause+=(FindPause()/samprate)*1000;
			   WriteTZXPause(mspause);
			   header.type=0xff;
			   subblocks=0;
			   PrintBlockInfo(0,grandtotal);
			   WriteTZXGroupEnd();
			 } break;
		case 5 : {
			   byte pnum;
			   dword grandtotal=0;
			   word bytes;
			   word mswave,waves=0;
			   if (!blockskip) blockskip=4;
			   for (blocks=1; blocks<blockskip+1; blocks++)
			       {
				 bytes=ReadNormalBlock();
				 if (DataLoadOK) PrintBlockInfo(nxtloadpos,bytes-2);
				 WriteTZXNormalBlock(bytes);
			       }
			   PushFilePointers();
			   printf("Reading waves...           \xd");
			   do {
				mswave=WaveSearch();
				if ((mswave<100 && waves) || mswave>170) break;
				else waves++;
			      }
			   while (blocks==blocks);
			   if (waves<100)
			      {
				PopFilePointers();
				lseek(inpfile.handle,filepos,SEEK_SET);
				inpfile.bufpoint=0;
				printf("Reading tone...         \xd");
				DecryptToneSearch();
				if (msbeep<15000) { strcpy(errstring,"Decryption tone too short"); Error(8); };
				header.type=0x0c;
				if (hifi) WriteTZXGroupStart();
				WriteDecryptTone();
				PrintBlockInfo(0,msbeep);
				xorbyte=0xee; addbyte=0x11;
			      }
			   else {
				  RemoveFilePointers();
				  mspause=mswave+(FindPause()/samprate)*1000;
				  header.type=0x0b;
				  if (hifi) WriteTZXGroupStart();
				  WriteDecryptWaves(waves);
				  WriteTZXPause(mspause);
				  PrintBlockInfo(0,waves);
				  xorbyte=0x43; addbyte=0x77;
				}
			   if (hifi) WriteTZXGroupEnd();
			   bit0.freq=3100;
			   bit1.freq=1550;
			   SetTimings();
			   header.type=0x51;
			   WriteTZXGroupStart();
			   bytes=ReadSpeedlock47FirstBlock();
			   if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
			   if (nxtloadpos>0xc000) slockloadaddress=0xfec5;
			   else slockloadaddress=0xbec5;
			   PrintBlockInfo(slockloadaddress,bytes-1);
			   WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
			   blocks++;
			   pulseseq[0]=sync.tstates; pulseseq[1]=sync.tstates;
			   for (pnum=0; pnum<7; pnum++)
			       {
				 word plen=3500000/(2*compositesync[pnum]);
				 pulseseq[2*pnum+2]=plen;
				 pulseseq[2*pnum+3]=plen;
			       }
			   WriteTZXPulseSequence(16);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   WriteTZXGroupEnd();
			   DecodeSpeedlockTable(bytes,0);
			   header.type=0x52;
			   PrintBlockInfo(0,0);
			   WriteTZXGroupStart();
			   nxtloadpos=16384; expectedlength=6144; rampage=0x10;
			   header.type=0x8;
			   subblocks=1;
			   grandtotal+=(bytes=ReadSpeedlock47FirstBlock());
			   PrintBlockInfo(nxtloadpos,bytes);
			   WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
			   pulseseq[0]=sync.tstates; pulseseq[1]=sync.tstates;
			   for (pnum=0; pnum<7; pnum++)
			       {
				 word plen=3500000/(2*compositesync[pnum]);
				 pulseseq[2*pnum+2]=plen;
				 pulseseq[2*pnum+3]=plen;
			       }
			   WriteTZXPulseSequence(16);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   GetLoadingParameters(bytes);
			   if (!SpeedEndBlock)
			      {
				if (!MidSyncSearch()) { strcpy(errstring,"Bad sync pulse detection or missing mid-sync"); Error(8); }
				bit0.freq=3100; bit1.freq=1550;
				SetTimings();
			      }
			   do {
				subblocks++;
				grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,expectedlength));
				PrintBlockInfo(nxtloadpos,bytes);
				WriteTZXPureData(8,bytes);
				GetLoadingParameters(bytes);
				if (!MidSyncSearch()) { strcpy(errstring,"Bad sync pulse detection or missing mid-sync"); Error(8); };
				bit0.freq=3100; bit1.freq=1550;
				SetTimings();
			      }
			   while (!SpeedEndBlock);
			   subblocks++;
			   nxtloadpos+=bytes;
			   grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,0));
			   PrintBlockInfo(nxtloadpos,bytes-1);
			   WriteTZXPureData(8,bytes);
			   if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); };
			   mspause+=(FindPause()/samprate)*1000;
			   WriteTZXPause(mspause);
			   header.type=0xff;
			   subblocks=0;
			   PrintBlockInfo(0,grandtotal);
			   WriteTZXGroupEnd();
			 } break;
		case 6 : {
			   byte pnum;
			   dword grandtotal=0;
			   word bytes;
			   if (!blockskip) blockskip=4;
			   for (blocks=1; blocks<blockskip+1; blocks++)
			       {
				 bytes=ReadNormalBlock();
				 if (DataLoadOK) PrintBlockInfo(nxtloadpos,bytes-2);
				 WriteTZXNormalBlock(bytes);
			       }
			   header.type=0x0c;
			   WriteTZXGroupStart();
			   DecryptToneSearch();
			   WriteDecryptTone();
			   PrintBlockInfo(0,msbeep);
			   WriteTZXGroupEnd();
			   bit0.freq=2450;
			   bit1.freq=1225;
			   SetTimings();
			   header.type=0x61;
			   WriteTZXGroupStart();
			   bytes=ReadSpeedlock47FirstBlock();
			   if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); };
			   if (nxtloadpos>0xc000) slockloadaddress=0xfec5;
			   else slockloadaddress=0xbec5;
			   PrintBlockInfo(slockloadaddress,bytes-1);
			   WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
			   blocks++;
			   pulseseq[0]=sync.tstates; pulseseq[1]=sync.tstates;
			   for (pnum=0; pnum<7; pnum++)
			       {
				 word plen=3500000/(2*compositesync[pnum]);
				 pulseseq[2*pnum+2]=plen;
				 pulseseq[2*pnum+3]=plen;
			       }
			   WriteTZXPulseSequence(16);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   WriteTZXGroupEnd();
			   xorbyte=0xee; addbyte=0x11;
			   DecodeSpeedlockTable(bytes,0);
			   header.type=0x62;
			   PrintBlockInfo(0,0);
			   WriteTZXGroupStart();
			   nxtloadpos=16384; expectedlength=6144; rampage=0x10;
			   header.type=0x8;
			   subblocks=1;
			   grandtotal+=(bytes=ReadSpeedlock47FirstBlock());
			   PrintBlockInfo(nxtloadpos,bytes);
			   WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
			   pulseseq[0]=sync.tstates; pulseseq[1]=sync.tstates;
			   for (pnum=0; pnum<7; pnum++)
			       {
				 word plen=3500000/(2*compositesync[pnum]);
				 pulseseq[2*pnum+2]=plen;
				 pulseseq[2*pnum+3]=plen;
			       }
			   WriteTZXPulseSequence(16);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   GetLoadingParameters(bytes);
			   if (!SpeedEndBlock)
			      {
				if (!MidSyncSearch()){ strcpy(errstring,"Bad sync pulse detection or missing mid-sync"); Error(8); }
				bit0.freq=2450; bit1.freq=1225;
				SetTimings();
			      }
			   do {
				subblocks++;
				grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,expectedlength));
				PrintBlockInfo(nxtloadpos,bytes);
				WriteTZXPureData(8,bytes);
				GetLoadingParameters(bytes);
				if (!MidSyncSearch()) { strcpy(errstring,"Bad sync pulse detection or missing mid-sync"); Error(8); }
				bit0.freq=2450; bit1.freq=1225;
				SetTimings();
			      }
			   while (!SpeedEndBlock);
			   subblocks++;
			   nxtloadpos+=bytes;
			   grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,15));
			   PrintBlockInfo(nxtloadpos,bytes-1);
			   WriteTZXPureData(8,bytes);
			   if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
			   mspause+=(FindPause()/samprate)*1000;
			   WriteTZXPause(mspause);
			   header.type=0xff;
			   subblocks=0;
			   PrintBlockInfo(0,grandtotal);
			   WriteTZXGroupEnd();
			 } break;
		case 7 : {
			   byte pnum;
			   dword grandtotal=0;
			   word bytes;
			   if (!blockskip) blockskip=2;
			   for (blocks=1; blocks<blockskip+1; blocks++)
			       {
				 bytes=ReadNormalBlock();
				 if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
				 PrintBlockInfo(nxtloadpos,bytes-2);
				 WriteTZXNormalBlock(bytes);
			       }
			   FindLoadAddress(bytes);
			   bit0.freq=2450;
			   bit1.freq=1225;
			   SetTimings();
			   header.type=0x71;
			   WriteTZXGroupStart();
			   bytes=ReadSpeedlock47FirstBlock();
			   if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
			   PrintBlockInfo(slockloadaddress,bytes-1);
			   WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
			   blocks++;
			   pulseseq[0]=sync.tstates; pulseseq[1]=sync.tstates;
			   for (pnum=0; pnum<7; pnum++)
			       {
				 word plen=3500000/(2*compositesync[pnum]);
				 pulseseq[2*pnum+2]=plen;
				 pulseseq[2*pnum+3]=plen;
			       }
			   WriteTZXPulseSequence(16);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   WriteTZXGroupEnd();
			   xorbyte=0xc1; addbyte=0x11;
			   DecodeSpeedlockTable(bytes,0);
			   header.type=0x72;
			   PrintBlockInfo(0,0);
			   WriteTZXGroupStart();
/*                         nxtloadpos=16384; expectedlength=6144; */
			   rampage=0x10;
			   header.type=0x8;
			   subblocks=1;
			   grandtotal+=(bytes=ReadSpeedlock47FirstBlock());
			   PrintBlockInfo(nxtloadpos,bytes);
			   WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
			   pulseseq[0]=sync.tstates; pulseseq[1]=sync.tstates;
			   for (pnum=0; pnum<7; pnum++)
			       {
				 word plen=3500000/(2*compositesync[pnum]);
				 pulseseq[2*pnum+2]=plen;
				 pulseseq[2*pnum+3]=plen;
			       }
			   WriteTZXPulseSequence(16);
			   saveslockflag=1;
			   WriteTZXPureData(slockflagbits+1,1);
			   saveslockflag=0;
			   WriteTZXPureData(8,bytes);
			   GetLoadingParameters(bytes);
			   if (!SpeedEndBlock)
			      {
				if (!MidSyncSearch()) { strcpy(errstring,"Bad sync pulse detection or missing mid-sync"); Error(8); }
				bit0.freq=2450; bit1.freq=1225;
				SetTimings();
			      }
			   do {
				subblocks++;
				grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,expectedlength));
				PrintBlockInfo(nxtloadpos,bytes);
				WriteTZXPureData(8,bytes);
				GetLoadingParameters(bytes);
				if (!MidSyncSearch()) { strcpy(errstring,"Bad sync pulse detection or missing mid-sync"); Error(8); }
				bit0.freq=2450; bit1.freq=1225;
				SetTimings();
			      }
			   while (!SpeedEndBlock);
			   subblocks++;
			   nxtloadpos+=bytes;
			   grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,15));
			   PrintBlockInfo(nxtloadpos,bytes-1);
			   WriteTZXPureData(8,bytes);
			   if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
			   mspause+=(FindPause()/samprate)*1000;
			   WriteTZXPause(mspause);
			   header.type=0xff;
			   subblocks=0;
			   PrintBlockInfo(0,grandtotal);
			   WriteTZXGroupEnd();
			 } break;
	      }
     }
  if (zeta)
     {
       dword grandtotal=0;
       word bytes;
       printf("þ Operating in ZETALOAD mode\n\n");
       if (!blockskip) blockskip=2;
       for (blocks=1; blocks<blockskip+1; blocks++)
	   {
	     bytes=ReadNormalBlock();
	     if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
	     else PrintBlockInfo(nxtloadpos,bytes-2);
	     WriteTZXNormalBlock(bytes);
	   }
       FindLoadAddress(bytes-2);
       slockloadaddress--;
       header.type=0xb1;
       WriteTZXGroupStart();
       bytes=ReadZetaLoadFirstBlock();
       if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
       PrintBlockInfo(slockloadaddress,bytes-1);
       WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
       blocks++;
       pulseseq[0]=sync.tstates; pulseseq[1]=sync.tstates;
       WriteTZXPulseSequence(2);
       saveslockflag=1;
       bit0.freq=2044; bit1.freq=1022; SetTimings();
       WriteTZXPureData(slockflagbits+1,1);
       saveslockflag=0;
       bit0.freq=3100; bit1.freq=1550; SetTimings();
       WriteTZXPureData(8,bytes);
       WriteTZXGroupEnd();
       DecodeSpeedlockTable(bytes,0);
       header.type++;
       WriteTZXGroupStart();
       PrintBlockInfo(nxtloadpos,bytes);
       subblocks=1;
       header.type=8;
       nxtloadpos=16384; expectedlength=6144; rampage=0x10;
       bytes=ReadZetaLoadFirstBlock();
       if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
       WriteTZXTone(pilot.tstates,(pilotlength/pilot.samples)*2);
       pulseseq[0]=sync.tstates; pulseseq[1]=sync.tstates;
       WriteTZXPulseSequence(2);
       saveslockflag=1;
       bit0.freq=2044; bit1.freq=1022; SetTimings();
       WriteTZXPureData(slockflagbits+1,1);
       saveslockflag=0;
       bit0.freq=3100; bit1.freq=1550; SetTimings();
       WriteTZXPureData(8,bytes);
       slockflagbits=6;
       bit0.freq=2044; bit1.freq=1022; SetTimings();
       ReadSpeedlockFlag(bit0.samples,bit1.samples);
       if (slockflag!=2*0x5a) { strcpy(errstring,"Bad sync pulse detection or wrong flag byte"); Error(8); }
       saveslockflag=1;
       WriteTZXPureData(slockflagbits+1,1);
       saveslockflag=0;
       bit0.freq=3100; bit1.freq=1550; SetTimings();
       PrintBlockInfo(nxtloadpos,bytes);
       GetLoadingParameters(bytes);
       do {
	    subblocks++;
	    grandtotal+=(bytes=DataRead(bit0.samples,bit1.samples,expectedlength));
	    PrintBlockInfo(nxtloadpos,bytes);
	    WriteTZXPureData(8,bytes);
	    GetLoadingParameters(bytes);
	    if (!SpeedEndBlock)
	       {
		 bit0.freq=2044; bit1.freq=1022; SetTimings();
		 ReadSpeedlockFlag(bit0.samples,bit1.samples);
		 if (slockflag!=2*0x5a) { strcpy(errstring,"Bad sync pulse detection or wrong flag byte"); Error(8); }
		 saveslockflag=1;
		 WriteTZXPureData(slockflagbits+1,1);
		 saveslockflag=0;
		 bit0.freq=3100; bit1.freq=1550; SetTimings();
	       }
	  }
       while (!SpeedEndBlock);
       bytes=DataRead(bit0.samples,bit1.samples,1);
       WriteTZXPureData(8,bytes);
       if (!DataLoadOK) { strcpy(errstring,"Bad sync pulse detection or data error"); Error(8); }
       mspause+=(FindPause()/samprate)*1000;
       WriteTZXPause(mspause);
       header.type=0xff;
       subblocks=0;
       PrintBlockInfo(0,grandtotal);
       WriteTZXGroupEnd();
     }

/*       slockflagbits=6;
       ReadSpeedlockFlag(bit0.samples,bit1.samples);
       if (MirrorByte(slockflag)!=2*0x5a) Error(8); */
/*       do {
	    GetLoadingParameters(bytes);
	    grandtotal+=(bytes=DataRead(bit1.samples,bit0.samples);
	  }

     }  */
  if (blocks>1 && !blocktype) End();
  if (blocks>1) printf("\n");
  printf("þ Operating in RAW mode");
  if (blocks>1) printf(" for last %lu samples",inpfile.length-totalread-(dword)(samprate*mspause/1000));
  printf("\n\n");
  do RawRead();
  while (blocktype);
  End();
}
