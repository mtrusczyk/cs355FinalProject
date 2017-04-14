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

int CURRENT_STATE;
char *RECIPES_LIST[30];
int NUM_OF_RECIPES;

void main_menu();
void load_recipes(char[]);
void view_recipes(int);

int main()
{
	CURRENT_STATE = 0;

	tty_mode(0);
	set_cr_noecho_mode();

	main_menu();
	tty_mode(1);
}

void main_menu()
{
	system("clear");
	printf("Welcome to the NFC CookBook...\n\n");
	printf("\t1) Scan card\n");
	printf("\t2) Assign new card\n");
	printf("\t3) View recipes\n\n");

	int input = 0;
	while (input < '0' || input > '3')
	{
		input = getchar();
	}

	switch(input){
		case '1' : printf("1\n"); break;
		case '2' : printf("2\n"); break;
		case '3' : load_recipes("./recipes");view_recipes(0); break;
	}
}

void view_recipes(int pagenum)
{
	system("clear");
	printf("Recipes on file:\n\n");

	if (NUM_OF_RECIPES < 1 || NUM_OF_RECIPES == NULL)
	{
		printf("No recipes found...\n\n");
		printf("\t0) Back to Main Menu\n");

		int input = 0;
		while (input != 48)
		{
			input = getchar();
		}
		main_menu();
	}
	else {
		int i;
		for (i = 0; i < NUM_OF_RECIPES; i = i + 1)
		{
			printf("\t%d) %s\n", (i + 1), RECIPES_LIST[i + 7 * pagenum]);
		}

		if (NUM_OF_RECIPES > (7 * (pagenum + 1)))
		{
			printf("\t9) Next Page\n");
		}
		printf("\t0) Back to Main Menu\n\n");	

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
