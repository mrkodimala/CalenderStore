#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include<time.h>
struct node
{
	char msg[128];
	int msg_id;
	node *next;
}*flist,*alist,*printid;

struct bufserv{
	
		int userId;
		int forumId;
		int msgId;
		int commentId;
		int choice;
		char *forumname;
		char msg[128];
}buf1;

bool flag=true;
int mid = 0;
int count1 =0;
char *Data[100];
int count=1;
int values[100];
DWORD WINAPI SocketHandler(void*);
void replyto_client(char *buf, int *csock);

void socket_server() {

	//The port you want the server to listen on
	int host_port= 1101;

	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 2, 2 );
 	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 || ( LOBYTE( wsaData.wVersion ) != 2 ||
		    HIBYTE( wsaData.wVersion ) != 2 )) {
	    fprintf(stderr, "No sock dll %d\n",WSAGetLastError());
		goto FINISH;
	}

	//Initialize sockets and set options
	int hsock;
	int * p_int ;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		printf("Error initializing socket %d\n",WSAGetLastError());
		goto FINISH;
	}
	
	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
		printf("Error setting options %d\n", WSAGetLastError());
		free(p_int);
		goto FINISH;
	}
	free(p_int);

	//Bind and listen
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);
	
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY ;
	
	/* if you get error in bind 
	make sure nothing else is listening on that port */
	if( bind( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
		fprintf(stderr,"Error binding to socket %d\n",WSAGetLastError());
		goto FINISH;
	}
	if(listen( hsock, 10) == -1 ){
		fprintf(stderr, "Error listening %d\n",WSAGetLastError());
		goto FINISH;
	}
	
	//Now lets do the actual server stuff

	int* csock;
	sockaddr_in sadr;
	int	addr_size = sizeof(SOCKADDR);
	
	while(true){
		printf("waiting for a connection\n");
		csock = (int*)malloc(sizeof(int));
		
		if((*csock = accept( hsock, (SOCKADDR*)&sadr, &addr_size))!= INVALID_SOCKET ){
			//printf("Received connection from %s",inet_ntoa(sadr.sin_addr));
			CreateThread(0,0,&SocketHandler, (void*)csock , 0,0);
		}
		else{
			fprintf(stderr, "Error accepting %d\n",WSAGetLastError());
		}
	}

FINISH:
;
}

const int blocksize = 256;
const int datastart = 8192;
struct BitVector{
	int values[1024];
};

int GiveFreeBlock()
{
	int s = 1;
	int flag = 0;
	int count = 0;
	int j = 0;
	struct BitVector b;
	FILE *file = fopen("store.bin", "r+b");
	fread(&b, sizeof(struct BitVector), 1, file);
	for (int i = 0; i < 16384; i++)
	{
		int v = b.values[i];
		j = 1;
		s = 1;
		if (v < 4294967295)
		{
			//printf("%d\t", v);
			do{
				flag = v&s;
				count++;
				if (flag == 0)
				{
					b.values[i] = v^s;
					printf("\nB value=%d\n", b.values[i]);
					fseek(file, 0, SEEK_SET);
					fwrite(&b, sizeof(struct BitVector), 1, file);
					fclose(file);
					return datastart + (count * blocksize);
				}
				s = s << 1;
				j++;
			} while (j < 32);
		}
		else{
			count = count + 31;
		}
	}
	fclose(file);
	return 0;
}


void FreeBlock(int blockno)
{
	FILE *file = fopen("store.bin", "rb+");
	struct BitVector b;
	fread(&b, sizeof(struct BitVector), 1, file);
	blockno = blockno - datastart;
	blockno = blockno / 1024;
	int d = blockno / 32;
	int r = blockno % 32;
	unsigned int s = 1;
	unsigned int v = b.values[d];
	int i;
	if (d == 0)
		i = 2;
	else
		i = 0;
	for (; i <= r; i++)
	{
		s = s << 1;
	}
	b.values[d] = v^s;
	fseek(file, 0, SEEK_SET);
	fwrite(&b, sizeof(struct BitVector), 1, file);
	fclose(file);
}


struct User{
	char name[32];
};

struct UsersData{
	struct User users[31];
	int count;
};

struct Category{
	char name[28];
	int peopleoffset;
};

struct CatCollection{
	struct Category categories[31];
	int count;
};

struct Person{
	char name[32];
	char role[32];
	char phonenumber[12];
	int appointments;
};

struct PersonsTable{
	int persons[61];
	int count;
	int nexttbleadd;
	int prevtbleadd;
};


struct Appointment{
	char name[32];
	char date[16];
	char purpose[128];
	int nextaddress;
};

void todaysdate(char *date)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	int dt = tm.tm_mday;
	int mm = tm.tm_mon + 1;
	int yy = tm.tm_year + 1900;
	char day[5], month[5], year[8];
	itoa(dt, day, 10);
	itoa(mm, month, 10);
	itoa(yy, year, 10);
	strcpy(date, day);
	strcat(date, "-");
	strcat(date, month);
	strcat(date, "-");
	strcat(date, year);
}

int noofdays(char *date)
{
	char temp[10];
	int i;
	int offset = 0;
	for (i = 0; date[i] != '-'; i++)
		temp[offset++] = date[i];
	temp[offset] = '\0';
	int day = atoi(temp);
	i++;
	offset = 0;
	for (; date[i] != '-'; i++)
		temp[offset++] = date[i];
	temp[offset] = '\0';
	int month = atoi(temp);
	i++;
	offset = 0;
	for (; date[i] != '\0'; i++)
		temp[offset++] = date[i];
	temp[offset] = '\0';
	int year = atoi(temp);
	int noofdays;
	if (month <= 2)
	{
		noofdays = (year - 1) / 4 - (year - 1) / 100 + (year - 1) / 400;
	}
	else{
		noofdays = (year) / 4 - (year) / 100 + (year) / 400;
	}
	for (i = 1; i < month; i++)
	{
		if (i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10 || i == 12)
			noofdays = noofdays + 31;
		if (i == 4 || i == 6 || i == 9 || i == 11)
			noofdays = noofdays + 30;
		if (i == 2)
		{
			if (year / 4 == 0 || year / 400 == 0)
				noofdays = noofdays + 29;
			else
				noofdays = noofdays + 28;
		}
	}
	return noofdays + (year * 365)+day;
}

int noofdaysbetweentwodates(char *date1, char *date2)
{
	int n1 = noofdays(date1);
	int n2 = noofdays(date2);
	return n1-n2;
}



int cur_cat_number;
int cur_user_number;
int cur_person_number;
int cur_person_tble_offset;
void ViewCategoryPersons(char *command)
{
	FILE *file = fopen("store.bin", "rb+");
	struct CatCollection c;
	fseek(file, 5120, SEEK_SET);
	fread(&c, sizeof(c), 1, file);
	struct Category cat = c.categories[cur_cat_number];
	int offset = cat.peopleoffset;
	if (offset <= 0)
	{
		strcpy(command, "*addperson$name$role$phonenumber$#");
	}
	else{
		struct PersonsTable p;
		struct Person person;
		cur_person_tble_offset = offset;
		fseek(file, offset, SEEK_SET);
		fread(&p, sizeof(p), 1, file);
		strcpy(command, "@viewperson$\n\t");
		char temp[5];
		for (int i = 0; i < p.count; i++)
		{
			itoa(i + 1, temp, 10);
			strcat(command, temp);
			strcat(command, " . ");
			int personoffset = p.persons[i];
			fseek(file, personoffset, SEEK_SET);
			fread(&person, sizeof(person), 1, file);
			printf("\n%s\t%s\t%s\n",person.name,person.role,person.phonenumber);
			strcat(command, person.name);
			strcat(command, "\t");
			strcat(command, person.role);
			strcat(command, "\t");
			strcat(command, person.phonenumber);
			strcat(command, "\n\t");
		}
		strcat(command, "0 . Add New Person\n\t$0$");
		itoa(p.count, temp, 10);
		strcat(command, temp);
		strcat(command, "$$#");
	}
	fclose(file);
}

void ViewPersonAppoinments(char *command)
{
	FILE *file = fopen("store.bin", "rb+");
	struct PersonsTable p;
	fseek(file, cur_person_tble_offset, SEEK_SET);
	fread(&p, sizeof(p), 1, file);
	int personoffset = p.persons[cur_person_number];
	struct Person person;
	fseek(file, personoffset, SEEK_SET);
	fread(&person, sizeof(person), 1, file);
	int offset = person.appointments;
	if (offset == 0)
	{
		strcpy(command, "^addappoint$#");
	}
	else{
		strcpy(command, "@appointments$\n\t");
		struct Appointment a;
		int i = 0;
		char temp[5];
		while (offset > 0)
		{
			i++;
			fseek(file, offset, SEEK_SET);
			fread(&a, sizeof(a), 1, file);
			itoa(i, temp, 10);
			strcat(command, temp);
			strcat(command, " . ");
			strcat(command, a.name);
			strcat(command, "\t");
			strcat(command, a.date);
			strcat(command, "\t");
			strcat(command, a.purpose);
			strcat(command, "\n\t");
			offset = a.nextaddress;
		}
		strcat(command, "0.Enter New Appointment\n\t1.To Exit\n\t$0$1$$#");
	}
}

void ProcessAppointmentsReply(char *command)
{
	int i;
	for (i = 1; command[i] != '$'; i++);
	char option[5];
	i++;
	int offset = 0;
	for (; command[i] != '$'; i++)
		option[offset++] = command[i];
	option[offset] = '\0';
	int choice = atoi(option);
	if (choice == 0)
	{
		strcpy(command, "^addappoint$#");
	}
	else{
		strcpy(command, "Press any key to Exit\n#");
	}
}


void ProcessAddAppointment(char *command)
{
	int i;
	for (i = 1; command[i] != '$'; i++);
	char date[15];
	int offset = 0;
	i++;
	for (; command[i] != '$'; i++)
		date[offset++] = command[i];
	date[offset] = '\0';
	char purpose[50];
	offset = 0;
	i++;
	for (; command[i] != '$'; i++)
		purpose[offset++] = command[i];
	purpose[offset] = '\0';
	char todaydate[15];
	todaysdate(todaydate);
	int n = noofdaysbetweentwodates(date, todaydate);
	if (n<0)
	{
		strcpy(command,"You cannot Book Appointment for Previous Days#");
	}
	else if (n>180)
	{
		strcpy(command, "You Cannot Book Appointment Beyond Six months#");
	}
	else{
		FILE *file = fopen("store.bin", "rb+");
		struct PersonsTable p;
		fseek(file, cur_person_tble_offset, SEEK_SET);
		fread(&p, sizeof(p), 1, file);
		int personoffset = p.persons[cur_person_number];
		struct Person person;
		fseek(file, personoffset, SEEK_SET);
		fread(&person, sizeof(person), 1, file);
		int offset = person.appointments;
		int tempoffset;
		int flag = 0;
		struct Appointment app;
		int appoffset = GiveFreeBlock();
		if (offset == 0)
		{
			person.appointments = appoffset;
			fseek(file, personoffset, SEEK_SET);
			fwrite(&person, sizeof(person), 1, file);
		}
		else{
			tempoffset = offset;
			while (offset > 0)
			{
				fseek(file, offset, SEEK_SET);
				fread(&app, sizeof(app), 1, file);
				int k = noofdaysbetweentwodates(date, app.date);
				if (k == 0)
					flag = 1;
				tempoffset = offset;
				offset = app.nextaddress;
			}
			if (flag == 1)
			{
				strcpy(command,"This date is already Registered#");
				return;
			}
			app.nextaddress = appoffset;
			fseek(file, tempoffset, SEEK_SET);
			fwrite(&app, sizeof(app), 1, file);
		}

		struct UsersData u;
		fseek(file, 4096, SEEK_SET);
		fread(&u, sizeof(u), 1, file);
		char username[32];
		strcpy(username, u.users[cur_user_number].name);
		memset(&app, 0, sizeof(app));
		strcpy(app.date, date);
		strcpy(app.purpose, purpose);
		strcpy(app.name, username);
		fseek(file, appoffset, SEEK_SET);
		fwrite(&app, sizeof(app), 1, file);
		fclose(file);
		ViewPersonAppoinments(command);
	}
}


void ProcessViewPerson(char *command)
{
	int i;
	for (i = 1; command[i] != '$'; i++);
	char option[5];
	int offset = 0;
	i++;
	for (; command[i] != '$'; i++)
		option[offset++] = command[i];
	option[offset] = '\0';
	int choice = atoi(option);
	if (choice == 0)
	{
		strcpy(command, "*addperson$name$role$phonenumber$#");
	}
	else {
		cur_person_number = choice - 1;
		ViewPersonAppoinments(command);
		//strcpy(command, "Appointments will be processed soon#");
	}
}

void ProcessAddPerson(char *command)
{
	int i;
	for (i = 1; command[i] != '$'; i++);
	i++;
	char name[32];
	int offset = 0;
	for (; command[i] != '$'; i++)
		name[offset++] = command[i];
	name[offset] = '\0';
	char role[32];
	i++;
	offset = 0;
	for (; command[i] != '$'; i++)
		role[offset++] = command[i];
	role[offset] = '\0';
	i++;
	char phnenumber[12];
	offset = 0;
	for (; command[i] != '$'; i++)
		phnenumber[offset++] = command[i];
	phnenumber[offset] = '\0';
	struct Person person;
	memset(&person, 0, sizeof(person));
	strcpy(person.name, name);
	strcpy(person.role, role);
	strcpy(person.phonenumber, phnenumber);
	struct PersonsTable p;
	FILE *file = fopen("store.bin", "rb+");
	struct CatCollection c;
	fseek(file, 5120, SEEK_SET);
	fread(&c, sizeof(c), 1, file);
	struct Category cat = c.categories[cur_cat_number];
	offset = cat.peopleoffset;
	if (offset == 0)
	{
		offset = GiveFreeBlock();
		cat.peopleoffset = offset;
		c.categories[cur_cat_number] = cat;
		fseek(file, 5120, SEEK_SET);
		fwrite(&c, sizeof(c), 1, file);
	}
	fseek(file, offset, SEEK_SET);
	fread(&p, sizeof(p), 1, file);
	int personoffset = GiveFreeBlock();
	fseek(file, personoffset, SEEK_SET);
	fwrite(&person, sizeof(person), 1, file);
	p.persons[p.count++] = personoffset;
	fseek(file, offset, SEEK_SET);
	fwrite(&p, sizeof(p), 1, file);
	fclose(file);
	ViewCategoryPersons(command);
}


void ProcessViewCategory(char *command)
{
	int i;
	for (i = 1; command[i] != '$'; i++);
	char option[5];
	int offset = 0;
	i++;
	for (; command[i] != '$'; i++)
		option[offset++] = command[i];
	option[offset] = '\0';
	int choice = atoi(option);
	if (choice == 0)
	{
		strcpy(command, "@addcategory$\n\t1.Add Category\n\t$1$1$category$#");
	}
	else{
		//strcpy(command, "This will be Processed Soon#");
		cur_cat_number = choice - 1;
		ViewCategoryPersons(command);
	}
}

void ProcessSuccessLogin(char *command)
{
	struct CatCollection c;
	FILE *file = fopen("store.bin", "rb+");
	fseek(file, 5120, SEEK_SET);
	fread(&c, sizeof(struct CatCollection), 1, file);
	if (c.count == 0)
	{
		strcpy(command, "@addcategory$\n\t1.Add Category\n\t$1$1$category$#");
	}
	else{
		strcpy(command, "@viewcategory$\n\t");
		char temp[5];
		for (int i = 0; i < c.count; i++)
		{
			itoa(i + 1, temp, 10);
			strcat(command, temp);
			strcat(command, " . ");
			strcat(command, c.categories[i].name);
			strcat(command, "\n\t");
		}
		strcat(command, "0.Enter new Category$0$");
		itoa(c.count, temp, 10);
		strcat(command, temp);
		strcat(command, "$$#");
	}
}

void ProcessAddCategory(char *command)
{
	int i;
	for (i = 1; command[i] != '$'; i++);
	i++;
	char option[10];
	int offset = 0;
	for (; command[i] != '$'; i++)
		option[offset++] = command[i];
	option[offset] = '\0';
	char catname[32];
	i++;
	offset = 0;
	for (; command[i] != '$'; i++)
		catname[offset++] = command[i];
	catname[offset] = '\0';
	struct Category cat;
	memset(&cat, 0, sizeof(cat));
	strcpy(cat.name, catname);
	FILE *file = fopen("store.bin", "rb+");
	fseek(file, 5120, SEEK_SET);
	struct CatCollection c;
	fread(&c, sizeof(struct CatCollection), 1, file);
	c.categories[c.count++] = cat;
	fseek(file, 5120, SEEK_SET);
	fwrite(&c, sizeof(struct CatCollection), 1, file);
	fclose(file);
	ProcessSuccessLogin(command);
}


void ProcessMainScreen(char *command)
{
	int i;
	for (i = 1; command[i] != '$'; i++);
	i++;
	char option[10];
	int offset = 0;
	for (; command[i] != '$'; i++)
		option[offset++] = command[i];
	option[offset] = '\0';
	int choice = atoi(option);
	i++;
	offset = 0;
	char name[32];
	for (; command[i] != '$'; i++)
		name[offset++] = command[i];
	name[offset] = '\0';
	FILE *file = fopen("store.bin", "rb+");
	fseek(file, 4096, SEEK_SET);
	struct UsersData u;
	fread(&u, sizeof(struct UsersData), 1, file);
	if (choice == 1)
	{
		int flag = 0;
		for (i = 0; i < u.count; i++)
		{
			if (!strcmp(name, u.users[i].name))
			{
				cur_user_number = i;
				flag = 1;
				break;
			}
		}
		if (flag == 0)
		{
			strcpy(command, "Invalid Login#");
		}
		else if(flag==1){
			ProcessSuccessLogin(command);
		}
	}
	else if (choice == 2)
	{
		strcpy(u.users[u.count++].name, name);
		strcpy(command, "User Added Successfully#");
	}
	fseek(file, 4096, SEEK_SET);
	fwrite(&u, sizeof(struct UsersData), 1, file);
	fclose(file);
}

int processrecvbuf(char *command)
{
	if (command[0] != '$')
		return 0;
	char buffer[32];
	int offset = 0;
	for (int i = 1; command[i] != '$'; i++)
		buffer[offset++] = command[i];
	buffer[offset] = '\0';
	if (!strcmp("opened", buffer))
		return 1;
	if (!strcmp("mainscreen", buffer))
		return 2;
	if (!strcmp("addcategory", buffer))
		return 3;
	if (!strcmp("viewcategory", buffer))
		return 4;
	if (!strcmp("addperson", buffer))
		return 5;
	if (!strcmp("viewperson", buffer))
		return 6;
	if (!strcmp("addappoint", buffer))
		return 7;
	if (!strcmp("appointments", buffer))
		return 8;
	
}

void process_input(char *recvbuf, int recv_buf_cnt, int* csock)
{

	char replybuf[1024] = { '\0' };
	int k = processrecvbuf(recvbuf);
	if (k == 1)
	{
		printf("Client is started\n");
		strcpy(recvbuf, "@mainscreen$\n\n\tWelcome to Calender Store\n\t\t1.Login\n\t\t2.New User\n\t$1$2$name$#");
	}
	else if (k == 2)
	{
		ProcessMainScreen(recvbuf);
	}
	else if (k == 3)
	{
		ProcessAddCategory(recvbuf);
	}
	else if (k == 4)
	{
		ProcessViewCategory(recvbuf);
	}
	else if (k == 5)
	{
		ProcessAddPerson(recvbuf);
	}
	else if (k == 6)
	{
		ProcessViewPerson(recvbuf);
	}
	else if (k == 7)
	{
		ProcessAddAppointment(recvbuf);
	}
	else if (k == 8)
	{
		ProcessAppointmentsReply(recvbuf);
	}
	replyto_client(recvbuf, csock);
	replybuf[0] = '\0';
}

void replyto_client(char *buf, int *csock) {
	int bytecount;

	if ((bytecount = send(*csock, buf, strlen(buf), 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
	}
	printf("replied : %s", buf);
}

DWORD WINAPI SocketHandler(void* lp){
    int *csock = (int*)lp;

	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;

	memset(recvbuf, 0, recvbuf_len);
	if((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0))==SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free (csock);
		return 0;
	}

	//printf("Received bytes %d\nReceived string \"%s\"\n", recv_byte_cnt, recvbuf);
	process_input(recvbuf, recv_byte_cnt, csock);

    return 0;
}