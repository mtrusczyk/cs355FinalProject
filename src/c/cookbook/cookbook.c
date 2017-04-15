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
#include <curses.h>
#include	<stdlib.h>

int CURRENT_STATE;
char *RECIPES_LIST[30];
int NUM_OF_RECIPES;

void main_menu();
void load_recipes(char[]);
void view_recipes(int);
char *getUID();
char *get_hex(const uint8_t, const size_t, char[]);

int main()
{
	CURRENT_STATE = 0;

	tty_mode(0);
	set_cr_noecho_mode();

	WINDOW * mainwin;

    
    /*  Initialize ncurses  */

    if ( (mainwin = initscr()) == NULL ) {
	fprintf(stderr, "Error initialising ncurses.\n");
	exit(1);
    }

	main_menu();
	tty_mode(1);
}

void main_menu()
{
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

	int input = 0;
	while (input < '0' || input > '3')
	{
		input = getchar();
	}

	switch(input){
		case '1' : printf("%s",getUID()); break;
		case '2' : printf("2\n"); break;
		case '3' : load_recipes("./recipes");view_recipes(0); break;
	}
}

void view_recipes(int pagenum)
{
	clear();
	move(3,0);
	addstr("Recipes on file:");

	if (NUM_OF_RECIPES < 1 || NUM_OF_RECIPES == NULL)
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

		//printf("\t1) Print the number 3\n");
		//printf("\t2) Print the number 4\n");
		//printf("\t3) Print the number 5\n");
		//printf("\t4) Go back to main menu.\n\n");

		int input = 0;
		while (input < '0' || input > '9')
		{
			input = getchar();
		}
		

		switch(input){
			case '1' : printf("3\n"); break;
			case '2' : printf("4\n"); break;
			case '3' : printf("5\n"); break;
			case '4' :
			case '5' :
			case '6' :
			case '7' :
			case '9' : view_recipes(pagenum + 1); break;
			case '0' : main_menu(); break;
		}
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

#include <stdlib.h>
#include <nfc/nfc.h>

char *get_hex(const uint8_t *pbtData, const size_t szBytes, char[] UID)
{
  size_t  szPos;

  for (szPos = 0; szPos < szBytes; szPos++) {
    strcat(UID, pbtData[szPos]);
  }
}


char *getUID()
{
  nfc_device *pnd;
  nfc_target nt;
  char UID[8];

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
  (void)argc;

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
    printf("       UID (NFCID%c): ", (nt.nti.nai.abtUid[0] == 0x08 ? '3' : '1'));
    get_hex(nt.nti.nai.abtUid, nt.nti.nai.szUidLen, UID);
  }
  // Close NFC device
  nfc_close(pnd);
  // Release the context
  nfc_exit(context);
  return UID;
}
