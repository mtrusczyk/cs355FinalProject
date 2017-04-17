/*	@author:	Jeremy Dube
 *	@version:	0.1
 *	@purpose:	Provide command line interface for cookbook
 */

#include	<stdio.h>
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

int CURRENT_STATE;
char *RECIPES_LIST[30];
int NUM_OF_RECIPES;
int button = 0;
int pagenum;

void main_menu();
void load_recipes(char[]);
void view_recipes(int);
void print_recipe(char[]);
void getUID(unsigned char[]);
int  readButton(int pin);
void setupButton(int pin);
void waitForInput();

void get_hex(uint8_t *pbtData, size_t szBytes, unsigned char *uid)
{
  sprintf(uid, "%02x%02x%02x%02x",pbtData[0],pbtData[1],pbtData[2],pbtData[3]);
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

	main_menu();
	waitForInput();
}

void decodeState()
{
      	if (CURRENT_STATE == 1)
      	{
        	switch(button){
          		case 1: printf("1\n");button = 0; break;
          		case 2: printf("2\n"); break;
          		case 3: load_recipes("./recipes");view_recipes(0); break;
        	}
      	}

      	if (CURRENT_STATE == 2)
      	{
      		switch(button){
				case 1:button = 0; printf("3\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[0 + 3 * pagenum]); break;
				case 2:button = 0; printf("4\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[1 + 3 * pagenum]); break;
				case 3:button = 0; printf("5\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[2 + 3 * pagenum]); break;
				case 4:button = 0; view_recipes(pagenum++); break;
			}
      	}
      	if (CURRENT_STATE == 3)
      	{
		switch(button){
		      		case '1' : printf("3\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[0 + 3 * pagenum]); break;
				case '2' : printf("4\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[1 + 3 * pagenum]); break;
				case '3' : printf("5\n"); clear(); move(0,0); print_recipe(RECIPES_LIST[2 + 3 * pagenum]); break;
				case '4' : main_menu(); break;
		}
      	}
	delay(250);
}

void waitForInput()
{
  for(;;)
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

void view_recipes(int pagenum)
{
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

		int input = 0;
		while (input != 48)
		{
			input = getchar();
		}
		main_menu();
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
