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
#include	<sys/ioctl.h>
#include	<curl/curl.h>

#define BUFFERSIZE 10240
#define BUTTON1 0
#define BUTTON2 1
#define BUTTON3 2
#define BUTTON4 3
#define USERPASSWD ""
#define SERVER ""
#define WIDTH 58
#define HEIGHT 49
#define menuStart 45
char *choices[] = {     "Scan Card",
            "Assign New Card",
            "Exit"
          };


struct csv{
	char UID[12];
	char file[540];
};

int CURRENT_STATE;
char RECIPES_LIST[30][100];
int NUM_OF_RECIPES = 0;
char nexttext[BUFFERSIZE];
int button = 0, cont = 1;
int pagenum;
int NUM_OF_CSV = 0;
int startx = 0;
int starty = 0;
int displayedRecipes = 0;

struct csv uids[50];

WINDOW *mainwin;
MEVENT event;
WINDOW *menu_win;

void main_menu();
void load_recipes();
void view_recipes(int);
void open_recipe(char[]);
void print_recipe_page(char[]);
void getUID(unsigned char[]);
int  readButton(int pin);
void setupButton(int pin);
void waitForInput();
void readCsv(char[]);
int  nextUID(char[]);
void writeCSV(char[]);
void writeUID(char [], char []);
int  findUID(char[]);

void scanNFC()
{
	clear();
	mvprintw(HEIGHT/2, (WIDTH-strlen("PLEASE SCAN CARD"))/2,"PLEASE SCAN CARD");
	refresh();
}

void removeNFC()
{
	clear();
	mvprintw(HEIGHT/2, (WIDTH-strlen("PLEASE REMOVE CARD"))/2,"PLEASE REMOVE CARD");
	refresh();
}

void optionsWindow()
{
    int i;
    for(i = 0; i<WIDTH; i++)
    {
	mvaddch(menuStart,i,'_');
    }
    mvaddch(menuStart+1,WIDTH/3,'|');
    mvaddch(menuStart+2,WIDTH/3,'|');
    mvaddch(menuStart+3,WIDTH/3,'|');
    mvaddch(menuStart+1,2*WIDTH/3,'|');
    mvaddch(menuStart+2,2*WIDTH/3,'|');
    mvaddch(menuStart+3,2*WIDTH/3,'|');

    mvprintw(menuStart+2,WIDTH/6-2,"Next");
    mvprintw(menuStart+2,3*WIDTH/6-2,"Main");
    mvprintw(menuStart+2,5*WIDTH/6-2,"Exit");
}
int getOptionWindow(int x)
{
    if(x < WIDTH/3)
	return 1;

    else if(x<2*WIDTH/3)
	return 2;

    else
	return 3;
}

int getMouseInput(int *x, int *y){
	int c;
   while(1)
   {
        c = getch();
        switch(c)
        {    case KEY_MOUSE:
            if(getmouse(&event) == OK)
            {    /* When the user clicks left mouse button */
                if(event.bstate & BUTTON1_PRESSED)
                {
                	*x = event.x + 1;
                	*y = event.y + 1;
                	return 1;
                }
            }
    	}
    }
}

int getCurl(char *recipe, char *file_n)
{
  CURLcode ret;
  CURL *hnd;

  FILE *file = fopen( file_n, "w");

  char dir[1024];
  sprintf(dir,"sftp://%s/~/recipes/%s",SERVER,recipe);

  hnd = curl_easy_init();
  curl_easy_setopt(hnd, CURLOPT_URL, dir);
  curl_easy_setopt(hnd, CURLOPT_USERPWD, USERPASSWD);
  curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.38.0");
  curl_easy_setopt(hnd, CURLOPT_WRITEDATA, file);
  curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(hnd, CURLOPT_HEADER, 0L);

  ret = curl_easy_perform(hnd);

  curl_easy_cleanup(hnd);
  hnd = NULL;
  fclose(file);
  return (int)ret;
}

int putCurl(char *LOCAL_FILE, char *rname)
{
  CURL *curl;
  char REMOTE_URL[1024];
  CURLcode res;
  FILE *hd_src;
  struct stat file_info;
  curl_off_t fsize;
 
  sprintf(REMOTE_URL,"sftp://%s/~/recipes/%s",SERVER,rname);


  /* get the file size of the local file */ 
  if(stat(LOCAL_FILE, &file_info)) {
    printf("Couldnt open '%s': %s\n", LOCAL_FILE, strerror(errno));
    return 1;
  }
  fsize = (curl_off_t)file_info.st_size;
 
  /* get a FILE * of the same file */ 
  hd_src = fopen(LOCAL_FILE, "rb");
 
  /* In windows, this will init the winsock stuff */ 
  curl_global_init(CURL_GLOBAL_ALL);
 
  /* get a curl handle */ 
  curl = curl_easy_init();
  if(curl) {
 
    curl_easy_setopt(curl, CURLOPT_USERPWD, USERPASSWD);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.38.0");

    /* enable uploading */ 
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
 
    /* specify target */ 
    curl_easy_setopt(curl, CURLOPT_URL, REMOTE_URL);
 
    /* now specify which file to upload */ 
    curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
 
    /* Set the size of the file to upload (optional).  If you give a *_LARGE
       option you MUST make sure that the type of the passed-in argument is a
       curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
       make sure that to pass in a type 'long' argument. */ 
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)fsize);
 
    /* Now run off and do what you've been told! */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform(%d) failed: %s\n",res,curl_easy_strerror(res));
 
    /* clean up the FTP commands list */ 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }
  fclose(hd_src); /* close the local file */ 
 
  curl_global_cleanup();
  return 0;

}

void get_hex(uint8_t *pbtData, size_t szBytes, unsigned char *uid)
{
  sprintf(uid, "%02x%02x%02x%02x",pbtData[0],pbtData[1],pbtData[2],pbtData[3]);
}

void writeUID(char *uid, char *fil)
{
  	int i;
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


    /*  Initialize ncurses  */

    if ( (mainwin = initscr()) == NULL ) {
	fprintf(stderr, "Error initialising ncurses.\n");
	exit(1);
    }
    menu_win = newwin(HEIGHT, WIDTH, starty, startx);
    keypad(mainwin, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);

   /* getCurl("recipeList.txt","recipeList.txt");*/
    load_recipes();
    getCurl("UIDList.csv","file.csv");
    readCsv("./file.csv");
    chdir("./recipes");
    main_menu();
    waitForInput();
    chdir("..");
    writeCSV("./file.csv");
    putCurl("file.csv", "UIDList.csv");
    endwin();
}

void getRecipe(int i, char *j)
{
	strcpy(j,RECIPES_LIST[i]);
}

void decodeState(int x, int y)
{
      	if (CURRENT_STATE == 1)
      	{
		char uid[12];
		int i;
		if(y < HEIGHT/3)
		{
          		clear();
			move(0,0);
			button = 0;
			scanNFC();
			getUID(uid);
			removeNFC();
			getCurl(uids[findUID(uid)].file,"recipe.txt");
			open_recipe("recipe.txt");
		}

		else if (y < 2*HEIGHT/3)
		{
          		view_recipes(0);
			button = 0;
			CURRENT_STATE = 2;
		}

		else
			cont=0;
      	}

      	else if (CURRENT_STATE == 2)
      	{
		if (y>= menuStart)
		{
			int i = getOptionWindow(x);
			if (i ==1)
				view_recipes(++pagenum);
			if (i == 2)
				main_menu();
			if (i == 3)
				cont = 0;
		}

		else if (y > displayedRecipes*4+2)
		{
		}
		else
		{
	      		char uid[12], recipe[512];
			getRecipe((y-2)/4,recipe);
			scanNFC();
			getUID(uid);
			writeUID(uid,recipe);
			main_menu();
		}
      	}

      	else if (CURRENT_STATE == 5)
      	{
		if (y>= menuStart)
		{
			int i = getOptionWindow(x);
			if (i ==1)
				print_recipe_page(nexttext);
			if (i == 2)
				main_menu();
			if (i == 3)
				cont = 0;
		}
      	}
}

void waitForInput()
{
    while (cont)
    {
	int x = 0, y = 0;
    	if (getMouseInput(&x, &y)){
		decodeState(x,y);
    	}
    }

}

void main_menu()
{
	button = 0;
	CURRENT_STATE = 1;
	pagenum = 0;
	clear();
    box(menu_win,0,0);
    int x, y, i;
    x = 2;
    y = 2;

    // for (i = 1;i < HEIGHT-1; i++)
    // {
    //     mvwaddch(menu_win,i,WIDTH/2,'|');
    // }
    for (i = 1;i < WIDTH; i++)
    {
	mvaddch(starty,i + startx,'-');
	mvaddch(starty + HEIGHT - 1, i + startx, '-');
        mvaddch(HEIGHT/3 + starty,i + startx,'-');
        mvaddch(2*(HEIGHT/3) + starty,i + startx,'-');
    }
	for (i = 1;i<HEIGHT-1;i++)
	{
		mvaddch(i+starty,startx,'|');
		mvaddch(i+starty,WIDTH + startx, '|');
	}

    mvprintw((HEIGHT/6)+ starty, (WIDTH/2)-(strlen(choices[0])/2)+ startx, "%s", choices[0]);
    mvprintw(3*(HEIGHT/6)+ starty, (WIDTH/2)-(strlen(choices[1])/2) + startx, "%s", choices[1]);
    mvprintw(5*(HEIGHT/6)+ starty, (WIDTH/2)-(strlen(choices[2])/2) + startx, "%s", choices[2]);

    refresh();
}


void view_recipes(int page)
{
	pagenum = page;
	button = 0;
	clear();
	move(0,0);
	addstr("Recipes on file:");
	displayedRecipes = 0;
	optionsWindow();

	if (NUM_OF_RECIPES < 1 || NUM_OF_RECIPES == (int)NULL)
	{
		move(3,0);
		addstr("No recipes found...");
	}
	else {
		int i;
		char message[1024];
		for (i = 0; (i + 10*pagenum) < NUM_OF_RECIPES && i < 10; i = i + 1)
		{
			displayedRecipes++;
			move(2+4*i,4);
			sprintf(message,"%d) %s\n", (i + 1), RECIPES_LIST[i + 10 * pagenum]);
			addstr(message);
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

void load_recipes()
{
	int in_fd;
	char recipes[BUFFERSIZE];

	if ((in_fd = open("recipeList.txt", O_RDONLY)) == -1)
	{
		perror("Cannot open source file");
		return;
	}
	int n_chars = read(in_fd,recipes,BUFFERSIZE);
	int i,j = 0;

	for(i = 0; i < n_chars; i++)
	{
		if(recipes[i] == '\n')
		{
			strncpy(RECIPES_LIST[NUM_OF_RECIPES++],&recipes[i-j],j);
			/*RECIPES_LIST[NUM_OF_RECIPES][j] = '\0';*/
			j = 0;
			i++;
		}
		j++;
	}
}


void open_recipe(char filename[])
{
	CURRENT_STATE = 5;
	int in_fd, out_fd, n_chars;
	char buf[BUFFERSIZE];
	char cat[BUFFERSIZE];
	cat[0] = '\0';

	char file_path[1024];
	sprintf(file_path, "./%s", filename);

	if ((in_fd = open(file_path, O_RDONLY)) == -1)
	{
		perror("Cannot open source file");
		return;
	}

	while ((n_chars = read(in_fd, buf, BUFFERSIZE)) > 0)
	{
		strncat(cat, buf,n_chars);
	}
	strcat(cat,"\0");

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

	print_recipe_page(cat);
}

void print_recipe_page(char text[])
{
	button = 0;
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

		// Counting Lines
	int i, j = 0;
	int lines = 1;
	for (i = 0; i < strlen(text); i = i + 1)
	{
		if (++j == w.ws_col-1)
		{
			lines++;
			j = 0;
		}

		if(text[i] == '\n')
		{
			lines = lines + 1;
			j = 0;
		}
	}

	char part[BUFFERSIZE];

	// Lines per page logic (4 spaces for options)

	int rows = w.ws_row - 4;

	if (lines < rows)
	{
		clear();
		move(0,0);
		addstr(text);
		optionsWindow();
		refresh();
	}
	else {

		int j = 0, k = 0;
		for (i = 0; j < rows; i = i + 1)
		{
			if (++k == w.ws_col-1)
			{
				j++;
				k = 0;
				if(j>= rows)
					break;
			}
			if (text[i] == '\n')
			{
				j = j + 1;
				k = 0;
			}
		}

		int upto = i ;


		strncpy(part, text, upto);
		part[upto] = '\0';


		strcpy(nexttext, &text[upto]);
		clear();
		move(0,0);
		addstr(part);

		optionsWindow();

		refresh();
	}

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
