/*	@author:	Jeremy Dube
 *	@version:	0.1
 *	@purpose:	Provide command line interface for cookbook
 */

#include	<stdio.h>
#include	<unistd.h>
#include	<string.h>
#include	<signal.h>
#include	<termios.h>
#include	<fcntl.h>
#include	<dirent.h>
#include	<sys/types.h>
#include	<curses.h>
#include	<stdlib.h>
#include	<nfc/nfc.h>
#include 	<wiringPi.h>
#include	<errno.h>

#define BUFFERSIZE 4096
#define BUTTON1 0
#define BUTTON2 1
#define BUTTON3 2
#define BUTTON4 3

struct csv{
	char UID[9];
	char file[540];
};

int CURRENT_STATE;
char *RECIPES_LIST[30];
int NUM_OF_RECIPES;
int button = 0, cont = 1;
int pagenum;
int NUM_OF_CSV = 0;

struct csv uids[50];


void main_menu();
void load_recipes(char[]);
void view_recipes(int);
void print_recipe(char[]);
void getUID(unsigned char[]);
int  readButton(int pin);
void setupButton(int pin);
void waitForInput();
void readCsv(char[]);
int nextUID(char[]);
void writeCSV(char[]);
void writeUID(char [], char []);
int findUID(char[]);



void get_hex(uint8_t *pbtData, size_t szBytes, unsigned char *uid)
{
  sprintf(uid, "%02x%02x%02x%02x",pbtData[0],pbtData[1],pbtData[2],pbtData[3]);
}

 void writeUID(char *uid, char *fil)
 {
  	int i;
	printf("%s",fil);
  	if ((i = findUID(uid)) == -1)
  	{
  		strcpy(uids[NUM_OF_CSV].UID,uid);
  		strcpy(uids[NUM_OF_CSV++].file,fil);
  		strcat(uids[i].file,"\0");
  	}
  	else
  	{
  		strcpy(uids[i].UID,uid);
  		strcpy(uids[i].file,fil);
  		strcat(uids[i].file,"\0");

  	}
		printf("%s\n",uids[NUM_OF_CSV-1].file);
		printf("%d",i);
		delay(5000);
}


int main()
{
	CURRENT_STATE = 0;

	if (wiringPiSetup () < 0)
  	{
    	fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno)) ;
    	return 1 ;
  	}
  	setupButton(BUTTON1);
  	setupButton(BUTTON2);
  	setupButton(BUTTON3);
  	setupButton(BUTTON4);

	WINDOW * mainwin;


    /*  Initialize ncurses  */

    if ( (mainwin = initscr()) == NULL ) {
	fprintf(stderr, "Error initialising ncurses.\n");
	exit(1);
    }

    readCsv("./file.csv");
    main_menu();
    waitForInput();
    writeCSV("./file.csv");
    endwin();
}

void getRecipe(int i, char *j)
{
	strcpy(j,RECIPES_LIST[i]);
}

void decodeState()
{
      	if (CURRENT_STATE == 1)
      	{
		char uid[12];
		int i;
        	switch(button){
          		case 1:clear(); move(0,0); button = 0;getUID(uid);print_recipe(uids[findUID(uid)].file);  break;
          		case 2: load_recipes("./recipes");view_recipes(0);button = 0;CURRENT_STATE = 4; break;
          		case 3: load_recipes("./recipes");view_recipes(0);button = 0; break;
			case 4: cont = 0;
        	}
      	}

      	else if (CURRENT_STATE == 2)
      	{
      		switch(button){
				case 1:button = 0; printf("3\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[0 + 3 * pagenum]); break;
				case 2:button = 0; printf("4\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[1 + 3 * pagenum]); break;
				case 3:button = 0; printf("5\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[2 + 3 * pagenum]); break;
				case 4:button = 0; view_recipes(pagenum++); break;
			}
      	}
      	else if (CURRENT_STATE == 3)
      	{
			switch(button){
		      		case 1 : printf("3\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[0 + 3 * pagenum]); break;
				case 2 : printf("4\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[1 + 3 * pagenum]); break;
				case 3 : printf("5\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[2 + 3 * pagenum]); break;
				case 4 : main_menu(); break;
			}
      	}

      	else if (CURRENT_STATE == 4)
      	{
		printf("%d",button);
      		char uid[12], recipe[512];
			switch(button){
				case 1 : clear(); move(0,0);getRecipe(0+3*pagenum,recipe); getUID(uid); writeUID(uid,recipe); main_menu();break;
				case 2 : clear(); move(0,0);getRecipe(1+3*pagenum,recipe); getUID(uid); writeUID(uid,recipe); main_menu();break;
				case 3 : clear(); move(0,0);getRecipe(2+3*pagenum,recipe); getUID(uid); writeUID(uid,recipe); main_menu();break;
				case 4 : button = 0; main_menu(); break;
			}
      	}
	delay(250);
}

void waitForInput()
{
  while (cont)
  {
    if (button)
      decodeState();
    else
    {
      	if (!readButton(BUTTON1))
        	button = 1;
      	else if (!readButton(BUTTON2))
        	button = 2;
      	else if (!readButton(BUTTON3))
        	button = 3;

      	else if (!readButton(BUTTON4))
        	button = 4;

    }
  }

}

void main_menu()
{
	button = 0;
	CURRENT_STATE = 1;
	pagenum = 0;
	clear();
	move(0,0);
	addstr("Welcome to the NFC CookBook...");
	move(2,4);
	addstr("1) Scan card");
	move(3,4);
	addstr("2) Assign new card\n");
	move(4,4);
	addstr("3) View recipes\n\n");
	refresh();

}

void view_recipes(int page)
{
	pagenum = page;
	button = 0;
	CURRENT_STATE = 2;
	clear();
	move(3,0);
	addstr("Recipes on file:");

	if (NUM_OF_RECIPES < 1 || NUM_OF_RECIPES == (int)NULL)
	{
		move(5,0);
		addstr("No recipes found...");
		move(7,4);
		addstr("0) Back to Main Menu");
		refresh();

	}
	else {
		int i;
		char message[1024];
		for (i = 0; (i + 3*pagenum) < NUM_OF_RECIPES && i < 3; i = i + 1)
		{
			move(5+i,4);
			sprintf(message,"%d) %s\n", (i + 1), RECIPES_LIST[i + 3 * pagenum]);
			addstr(message);
		}

		if (NUM_OF_RECIPES > (3 * (pagenum + 1)))
		{
			move(5+i,4);
			addstr("9) Next Page\n");
		}
		else
		{
			CURRENT_STATE = 3;
			move(5+i,4);
			addstr("0) Back to Main Menu\n\n");
		}
		refresh();


		
	}
}


set_cr_noecho_mode()
/*
 * purpose: put file descriptor 0 into chr-by-chr mode and noecho mode
 *  method: use bits in termios
 */
{
	struct	termios	ttystate;

	tcgetattr( 0, &ttystate);		/* read curr. setting	*/
	ttystate.c_lflag    	&= ~ICANON;	/* no buffering		*/
	ttystate.c_lflag    	&= ~ECHO;	/* no echo either	*/
	ttystate.c_cc[VMIN]  	=  1;		/* get 1 char at a time	*/
	tcsetattr( 0 , TCSANOW, &ttystate);	/* install settings	*/
}

tty_mode(int how)
{
	static struct termios original_mode;
	static int            original_flags;
	static int            stored = 0;

	if ( how == 0 ){
		tcgetattr(0, &original_mode);
		original_flags = fcntl(0, F_GETFL);
		stored = 1;
	}
	else if ( stored ) {
		tcsetattr(0, TCSANOW, &original_mode);
		fcntl( 0, F_SETFL, original_flags);
	}
}

void load_recipes(char dirname[])
{
	DIR		*dir_ptr;
	struct dirent	*direntp;
	if ( ( dir_ptr = opendir( dirname ) ) == NULL )
		fprintf(stderr,"ls1: cannot open %s\n", dirname);
	else
	{
		int i;
		for (i = 0; ( direntp = readdir( dir_ptr ) ) != NULL; i = i + 1){
			if (!strcmp(direntp->d_name,".") || !strcmp(direntp->d_name,".."))
				{
					i--;
					continue;
				}
			RECIPES_LIST[i] = direntp->d_name;
			NUM_OF_RECIPES = i + 1;
		}

		closedir(dir_ptr);
	}
}

void print_recipe(char filename[])
{
	int in_fd, out_fd, n_chars;
	char buf[BUFFERSIZE];
	char cat[BUFFERSIZE];
	cat[0] = '\0';
	buf[0] = '\0';

	char file_path[1024];
	sprintf(file_path, "./recipes/%s", filename);

	if ((in_fd = open(file_path, O_RDONLY)) == -1)
	{
		perror("Cannot open source file");
		return;
	}

	while ((n_chars = read(in_fd, buf, BUFFERSIZE)) > 0)
	{
		strcat(cat, buf);
	}

	if (n_chars == -1)
	{
		perror("Read error");
		return;
	}

	if (close(in_fd) == -1)
	{
		perror("Error closing file");
		return;
	}

	addstr(cat);
	refresh();
	//printf("%s\n", cat);
}

int readButton (int pin)
{
	return digitalRead(pin);
}

void setupButton(int pin)
{
	wiringPiSetup();
	pinMode (pin,INPUT);
	pullUpDnControl(pin,PUD_UP);
}
int findUID(char *uid)
{
	int i;
	for (i = 0; i < NUM_OF_CSV; i ++)
	{
		if (!strcmp(uid,uids[i].UID))
			return i;
	}
	return -1;
}

void writeCSV(char *file_path)
{
	char buffer[BUFFERSIZE];
	buffer[0] = '\0';
	int i, in_fd;
	for(i = 0; i < NUM_OF_CSV; i++)
	{
		char line[540];
		sprintf(line,"%s;%s;\n\0",uids[i].UID,uids[i].file);
		strcat(buffer,line);
	}
	if ((in_fd = open(file_path, O_WRONLY)) == -1)
	{
		perror("Cannot open source file");
	}

	write(in_fd,buffer,strlen(buffer));
printf("%s %d",buffer,strlen(buffer));

	close(in_fd);

}


void readCsv(char  *file)
{
	char buf[BUFFERSIZE];
	char cat[BUFFERSIZE];
	int n_chars, in_fd;
	cat[0] = '\0';
	buf[0] = '\0';
	char file_path[1024];
	sprintf(file_path,"./%s",file);
	if ((in_fd = open(file_path, O_RDONLY)) == -1)
	{
		perror("Cannot open source file");
	}

	while ((n_chars = read(in_fd, buf, BUFFERSIZE)) > 0)
	{
		strcat(cat, buf);
	}

	close(in_fd);

	int i = 0, next;
	int lastRecord = 0;
	
	while ((next = nextUID(&buf[lastRecord]))>0)
	{
		printf("%d\n",next);
		if(next > 0)	
		{
			strncat(uids[i].UID,&buf[lastRecord],8);
			uids[i].UID[8] = '\0';
			strncpy(uids[i].file,&buf[lastRecord+9],next-9);
			strcat(uids[i].file, "\0");
			NUM_OF_CSV++;
		}
		i++;
		lastRecord+=(next+2);
	}
}

int nextUID(char *buffer)
{
	int i, flag = 1;
	
	for (i = 0; i < strlen(buffer); i++)
	{
		if (flag && buffer[i] == ';')
			flag = 0;
		else if (!flag && buffer[i] ==';')
			return i;
	}
	return -1;
}

void getUID(unsigned char *uid)
{
  nfc_device *pnd;
  nfc_target nt;

  // Allocate only a pointer to nfc_context
  nfc_context *context;

  // Initialize libnfc and set the nfc_context
  nfc_init(&context);
  if (context == NULL) {
    printf("Unable to init libnfc (malloc)\n");
    exit(EXIT_FAILURE);
  }


  // Display libnfc version
  const char *acLibnfcVersion = nfc_version();

  // Open, using the first available NFC device which can be in order of selection:
  //   - default device specified using environment variable or
  //   - first specified device in libnfc.conf (/etc/nfc) or
  //   - first specified device in device-configuration directory (/etc/nfc/devices.d) or
  //   - first auto-detected (if feature is not disabled in libnfc.conf) device
  pnd = nfc_open(context, NULL);

  if (pnd == NULL) {
    printf("ERROR: %s\n", "Unable to open NFC device.");
    exit(EXIT_FAILURE);
  }
  // Set opened NFC device to initiator mode
  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    exit(EXIT_FAILURE);
  }

  // Poll for a ISO14443A (MIFARE) tag
  const nfc_modulation nmMifare = {
    .nmt = NMT_ISO14443A,
    .nbr = NBR_106,
  };
  if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) > 0) {
    get_hex(nt.nti.nai.abtUid, nt.nti.nai.szUidLen, uid);
  }
  // Close NFC device
  nfc_close(pnd);
  // Release the context
  nfc_exit(context);
}
