#include "header.h"

#define COLUMN_CNT 12	//출력할 column 최대 갯수

//출력 시 사용할 columnWidth배열에서의 index
#define PID_IDX 0
#define USER_IDX 1
#define PR_IDX 2
#define NI_IDX 3
#define VIRT_IDX 4
#define RES_IDX 5
#define SHR_IDX 6
#define S_IDX 7
#define CPU_IDX 8
#define MEM_IDX 9
#define TIME_P_IDX 10
#define COMMAND_IDX 11

#define TOP_ROW 0				//top 출력할 행
#define TASK_ROW 1				//task 출력할 행
#define CPU_ROW 2				//cpu 출력할 행
#define MEM_ROW 3				//mem 출력할 행
#define COLUMN_ROW 6			//column 출력할 행

unsigned long cpuTimeTable[PID_MAX];	//cpu의 이전 시각 저장할 hash table

myProc procList[PROCESS_MAX];	//완성한 myProc의 포인터 저장 배열

myProc *sorted[PROCESS_MAX];	//procList를 cpu 순으로 sorting한 myProc 포인터 배열

int procCnt = 0;				//현재까지 완성한 myProc 갯수

time_t before;
time_t now;

unsigned long uptime;			//os 부팅 후 지난 시간
unsigned long beforeUptime = 0;	//이전 실행 시의 os 부팅 후 지난 시각
unsigned long memTotal;			//전체 물리 메모리 크기
unsigned int hertz;	 			//os의 hertz값 얻기(초당 context switching 횟수)

long double beforeTicks[TICK_CNT] = {0, };	//이전의 cpu ticks 저장하기 위한 배열

pid_t myPid;					//자기 자신의 pid
uid_t myUid;					//자기 자신의 uid
char myPath[PATH_LEN];			//자기 자신의 path
char myTTY[TTY_LEN];			//자기 자신의 tty

int ch;
int row, col;

//두 proc *를 cpu와 pid로 비교해 a < b인지 return하는 함수
bool isGreater(myProc *a, myProc *b)
{
	if(a->cpu < b->cpu)
		return true;
	else if(a->cpu > b->cpu)
		return false;
	else{
		if(a->pid > b->pid)
			return true;
		else return false;
	}
}

//procList를 cpu 순으로 sorting해 sorted 배열을 완성하는 함수
void sort_by_cpu(void)
{
	for(int i = 0; i < procCnt; i++)		//포인터 복사
		sorted[i] = procList + i;

	for(int i = procCnt - 1; i > 0; i--){ // Bubble Sorting
		for(int j = 0; j < i; j++){
			if(isGreater(sorted[j], sorted[j+1])){
				myProc *tmp = sorted[j];
				sorted[j] = sorted[j+1];
				sorted[j+1] = tmp;
			}
		}
	}
	return;
}

//실제 화면에 출력하는 함수
void print_mytop(void)
{
	uptime = get_uptime();			//os 부팅 후 지난 시각
    char buf[BUFFER_SIZE];

	/*****	1행 UPTIME 출력	*****/
	
	char nowStr[128];				//현재 시각 문자열
	memset(nowStr, '\0', 128);		//문자열 초기화
	struct tm *tmNow = localtime(&now); // tm 구조체는 현재 시간에 대한 정보를 포함하는 구조체
										//&now를 함으로써 현재시간 불러들임.
	sprintf(nowStr, "top - %02d:%02d:%02d ", tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec); //시,분,초 출력

	struct tm *tmUptime = localtime(&uptime); // os 부팅 후 지난 시각 정보 입력

	char upStr[128];				//uiptime 문자열
	memset(upStr, '\0', 128);
	if(uptime < 60 * 60) //1시간 미만일 경우 분만 출력
		sprintf(upStr, "%2d min", tmUptime->tm_min);
	else if(uptime < 60 * 60 * 24) //1시간 이상 하루 이하일 경우 시 , 분 출력
		sprintf(upStr, "%2d:%02d", tmUptime->tm_hour, tmUptime->tm_min); 
	else							//하루 이상일 경우 일,시,분 출력
		sprintf(upStr, "%3d days, %02d:%02d", tmUptime->tm_yday, tmUptime->tm_hour, tmUptime->tm_min);
	
	int users = 0;							//active user 수 구하기

	struct utmp *utmp;     // 접속중인 사용자에 대한 정보
	setutent();								// utmp 처음부터 읽는 함수 ! 
	while((utmp = getutent()) != NULL)		// /proc/utmp 파일에서 null일 때까지 읽어들이기
		if(utmp->ut_type == USER_PROCESS)	// ut_type이 USER일 경우에만 count 증가
			users++;

	FILE *loadAvgFp;
	long double loadAvg[3]; //값을 읽어들일 배열 ( loadAvg 가 소숫값)
 	if((loadAvgFp = fopen(LOADAVG, "r")) == NULL){	//파일이 없을시 에러처리
		fprintf(stderr, "fopen error for %s\n", LOADAVG);
		exit(1);
	}
	memset(buf, '\0', BUFFER_SIZE); //버퍼 초기화
	fgets(buf, BUFFER_SIZE, loadAvgFp); //	1분, 5분, 15분간의 시스템 부하율
	fclose(loadAvgFp);	//파일 포인터 닫음.
	sscanf(buf, "%Lf%Lf%Lf", &loadAvg[0], &loadAvg[1], &loadAvg[2]);  // 불러들인 파일에서 필요한 값 추출

	mvprintw(TOP_ROW, 0, "%sup %s, %d users, load average: %4.2Lf, %4.2Lf, %4.2Lf", nowStr, upStr, users, loadAvg[0], loadAvg[1], loadAvg[2]); 
	
	//화면의 TOP_ROW행(0행), 0열부터 시스템 현재 시간, OS가 살아있는 시간, 유저 세션수 를 출력한다.
	//로드은 CPU Load의 이동 평균를 표시
	/*****	2행 Task 출력	*****/
	char *ptr;
	//프로세스들에 대한 정보
	
	unsigned int total = 0, running = 0, sleeping = 0, stopped = 0, zombie = 0; // 프로세스 상태 변수
	total = procCnt;
	
	//main 함수에서 search_proc 함수 속 add_proc_list 함수를 통해 procList배열에 프로세스에 대한 정보가 저장 됨.

	for(int i = 0; i < procCnt; i++){
		if(!strcmp(procList[i].stat, "R"))  //헤더파일에서 선언한 myproc 구조체에 대한 배열들, stat은 프로세스 상태
			running++;
		else if(!strcmp(procList[i].stat, "D")) 
			sleeping++;
		else if(!strcmp(procList[i].stat, "S"))
			sleeping++;
		else if(!strcmp(procList[i].stat, "T"))
			stopped++;
		else if(!strcmp(procList[i].stat, "t"))
			stopped++;
		else if(!strcmp(procList[i].stat, "Z"))
			zombie++;
	}
	mvprintw(TASK_ROW, 0, "Tasks:  %4u total,  %4u running, %4u sleeping,  %4u stopped, %4u zombie", total, running, sleeping, stopped, zombie);
	//현재 프로세스들의 상태를 나타내어준다.

	/*****	3행 %CPU 출력	*****/
	
	long double us, sy, ni, id, wa, hi, si, st; // 각 영역에서의 cpu 사용률

	FILE *cpuStatFp;
	if((cpuStatFp = fopen(CPUSTAT, "r")) == NULL){				// /proc/stat fopen
		fprintf(stderr, "fopen error for %s\n", CPUSTAT);
		exit(1);
	}
	memset(buf, '\0', BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, cpuStatFp); 
	fclose(cpuStatFp);
	ptr = buf;
	while(!isdigit(*ptr)) ptr++;  // 숫자가 아닌 값은 넘김.

	long double ticks[TICK_CNT] = {0.0, }; //ticks 배열 초기화

	sscanf(ptr, "%Lf%Lf%Lf%Lf%Lf%Lf%Lf%Lf", ticks+0, ticks+1, ticks+2, ticks+3, ticks+4, ticks+5, ticks+6, ticks+7);	//ticks read
	//ptr에 저장된 정보를 ticks에 저장

	unsigned long tickCnt = 0;
	long double results[TICK_CNT] = {0.0, };			
	if(beforeUptime == 0){								//이전 실행 시의 os 부팅 후 지난 시각이 0일때
		 tickCnt = uptime * hertz;						//부팅 후 현재까지 일어난 context switching 횟수
		 for(int i = 0; i < TICK_CNT; i++)				//읽은 ticks 그대로 출력
			 results[i] = ticks[i]; 
	}
	else{
		tickCnt = (uptime - beforeUptime) * hertz;		//부팅 후 현재까지 일어난 context switching 횟수
		for(int i = 0; i < TICK_CNT; i++)
			results[i] = ticks[i] - beforeTicks[i];		//이전에 저장한 tick수를 빼서 출력
	}
	for(int i = 0; i < TICK_CNT; i++){
		results[i] = (results[i] / tickCnt) * 100;		//퍼센트로 저장
		if(isnan(results[i]) || isinf(results[i]))		//예외 처리
			results[i] = 0;
	}
	

	mvprintw(CPU_ROW, 0, "%%Cpu(s):  %4.1Lf us, %4.1Lf sy, %4.1Lf ni, %4.1Lf id, %4.1Lf wa, %4.1Lf hi, %4.1Lf si, %4.1Lf st", 
	results[0], results[2], results[1], results[3], results[4], results[5], results[6], results[7]);
	//사용중인 CPU사용률,  시스템이사용하는 CPU 사용률, nice정책에 의해 사용되는 CPU, 사용되지 않는 cpu ,입출력 대기상태 사용률
	//하드웨어 인터럽트 cpu 사용률, 소프트웨어 인터럽트 cpu 사용률, 가상화 환경에서 cpu 손실율

	beforeUptime = uptime;								//시간 갱신

	for(int i = 0; i < TICK_CNT; i++)					
		beforeTicks[i] = ticks[i]; //시간에 따라 다를 수 있는 ticks수에 대한 계산을 위한 저장


	/*****	4,5행 MEM SWAP출력	*****/
	//
	unsigned long memTotal, memFree, memUsed, memAvailable, buffers, cached, sReclaimable, swapTotal, swapFree, swapUsed;
	
    FILE *meminfoFp;

	//메모리에 대한 정보는 /proc/meminfo에 저장되어있다. 하나하나 필요한 정보를 읽는다.
    if ((meminfoFp = fopen(MEMINFO, "r")) == NULL){	// /proc/meminfo open
		fprintf(stderr, "fopen error for %s\n", MEMINFO);
        exit(1);
    }

	int i = 0;


	while(i < MEMINFO_MEM_TOTAL_ROW){	//memTotal read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while(!isdigit(*ptr)) ptr++;	//필요없는 값 

    sscanf(ptr, "%lu", &memTotal);		// /proc/meminfo의 1행에서 memFree read


	while(i < MEMINFO_MEM_FREE_ROW){	//memFree read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while(!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &memFree);	// /proc/meminfo의 2행에서 memFree read


	while(i < MEMINFO_MEM_AVAILABLE_ROW){	//memAvailable read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while(!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &memAvailable);	// /proc/meminfo의 3행에서 memAvailable read


	while(i < MEMINFO_BUFFERS_ROW){	//buffers read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while(!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &buffers);	// /proc/meminfo의 4행에서 buffers read


	while(i < MEMINFO_CACHED_ROW){	//cached read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while(!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &cached);	// /proc/meminfo의 5행에서 cached read


	while(i < MEMINFO_SWAP_TOTAL_ROW){	//swapTotal read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while(!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &swapTotal);	// /proc/meminfo의 15행에서 swapTotal read


	while(i < MEMINFO_SWAP_FREE_ROW){	//swapFree read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while(!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &swapFree);	// /proc/meminfo의 16행에서 swapFree read


	while(i < MEMINFO_S_RECLAIMABLE_ROW){	//sReclaimable read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while(!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &sReclaimable);	// /proc/meminfo의 23행에서 sReclaimable read

	memUsed = memTotal - memFree - buffers - cached - sReclaimable;		//memUsed 계산
	swapUsed = swapTotal - swapFree;	//swapUsed 계산

	//buff/cache 값은 buffers와 cached 값, 그리고 캐시 용도로 사용하는 메모리들이 포함된 sReclaimable 값을 합친 값이다.
	mvprintw(MEM_ROW, 0, "Kib Mem : %8lu total,  %8lu free,  %8lu used,  %8lu buff/cache", memTotal, memFree, memUsed, buffers+cached+sReclaimable); 
	mvprintw(MEM_ROW+1, 0, "Kib Swap: %8lu total,  %8lu free,  %8lu used,  %8lu avail Mem", swapTotal, swapFree, swapUsed, memAvailable); 

    fclose(meminfoFp);

	int columnWidth[COLUMN_CNT] = {					//column의 x축 길이 저장하는 배열. 헤더파일에서 선언한 x축의 값들이 저장 된다.
		strlen(PID_STR), strlen(USER_STR), strlen(PR_STR), strlen(NI_STR),
		strlen(VIRT_STR), strlen(RES_STR), strlen(SHR_STR), strlen(S_STR),
		strlen(CPU_STR), strlen(MEM_STR), strlen(TIME_P_STR), strlen(COMMAND_STR) };

	for(int i = 0; i < procCnt; i++){			//PID 최대 길이 저장
		sprintf(buf, "%lu", procList[i].pid); //proclist 에서 가져온 값
		if(columnWidth[PID_IDX] < strlen(buf))
			columnWidth[PID_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++)			//USER 최대 길이 저장
		if(columnWidth[USER_IDX] < strlen(procList[i].user)){
			columnWidth[USER_IDX] = strlen(procList[i].user);
		}

	for(int i = 0; i < procCnt; i++){			//PR 최대 길이 저장
		sprintf(buf, "%d", procList[i].priority);
		if(columnWidth[PR_IDX] < strlen(buf))
			columnWidth[PR_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			//NI 최대 길이 저장
		sprintf(buf, "%d", procList[i].nice);
		if(columnWidth[NI_IDX] < strlen(buf))
			columnWidth[NI_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			//VIRT 최대 길이 저장
		sprintf(buf, "%lu", procList[i].vsz);
		if(columnWidth[VIRT_IDX] < strlen(buf))
			columnWidth[VIRT_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			//RES 최대 길이 저장
		sprintf(buf, "%lu", procList[i].rss);
		if(columnWidth[RES_IDX] < strlen(buf))
			columnWidth[RES_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			//SHR 최대 길이 저장
		sprintf(buf, "%lu", procList[i].shr);
		if(columnWidth[SHR_IDX] < strlen(buf))
			columnWidth[SHR_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			//S 최대 길이 저장
		if(columnWidth[S_IDX] < strlen(procList[i].stat))
			columnWidth[S_IDX] = strlen(procList[i].stat);
	}


	for(int i = 0; i < procCnt; i++){			//CPU 최대 길이 저장
		sprintf(buf, "%3.1Lf", procList[i].cpu);
		if(columnWidth[CPU_IDX] < strlen(buf))
			columnWidth[CPU_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			//MEM 최대 길이 저장
		sprintf(buf, "%3.1Lf", procList[i].mem);
		if(columnWidth[MEM_IDX] < strlen(buf))
			columnWidth[MEM_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			//TIME 최대 길이 저장
		if(columnWidth[TIME_P_IDX] < strlen(procList[i].time))
			columnWidth[TIME_P_IDX] = strlen(procList[i].time);
	}

	for(int i = 0; i < procCnt; i++){			//COMMAND 최대 길이 저장
		if(columnWidth[COMMAND_IDX] < strlen(procList[i].command))
			columnWidth[COMMAND_IDX] = strlen(procList[i].command);
	}

	int startX[COLUMN_CNT] = {0, };				//각 column의 시작 x좌표

	int startCol = 0, endCol = 0;
	int maxCmd = -1;							//COMMAND 출력 가능한 최대 길이

	if(col >= COLUMN_CNT - 1){					//COMMAND COLUMN만 출력하는 경우 (우측 화살표 많이 누른 경우)
		startCol = COMMAND_IDX;
		endCol = COLUMN_CNT;
		maxCmd = COLS;							//COMMAND 터미널 너비만큼 출력 가능
	}
	else{
		int i;
		for(i = col + 1; i < COLUMN_CNT; i++){
			startX[i] = columnWidth[i-1] + 2 + startX[i-1];
			if(startX[i] >= COLS){				//COLUMN의 시작이 이미 터미널 너비 초과한 경우
				endCol = i;
				break;
			}
		}
		startCol = col;
		if(i == COLUMN_CNT){
			endCol = COLUMN_CNT;					//COLUMN 전부 출력하는 경우
			maxCmd = COLS - startX[COMMAND_IDX];	//COMMAND 최대 출력 길이: COMMAND 터미널 너비 - COMMAND 시작 x좌표
		}
	}
	

	/*****		6행 column 출력 시작	*****/
	
	attron(A_REVERSE);
	for(int i = 0; i < COLS; i++)
		mvprintw(COLUMN_ROW, i, " ");

	int gap = 0;

	//PID 출력
	if(startCol <= PID_IDX && PID_IDX < endCol){
		gap = columnWidth[PID_IDX] - strlen(PID_STR);	//PID의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[PID_IDX] + gap, "%s", PID_STR);	//우측 정렬
	}

	//USER 출력
	if(startCol <= USER_IDX && USER_IDX < endCol)
		mvprintw(COLUMN_ROW, startX[USER_IDX], "%s", USER_STR);	//좌측 정렬

	//PR 출력
	if(startCol <= PR_IDX && PR_IDX < endCol){
		gap = columnWidth[PR_IDX] - strlen(PR_STR);		//PR 의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[PR_IDX] + gap, "%s", PR_STR);	//우측 정렬
	}

	//NI 출력
	if(startCol <= NI_IDX && NI_IDX < endCol){
		gap = columnWidth[NI_IDX] - strlen(NI_STR);		//NI 의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[NI_IDX] + gap, "%s", NI_STR);	//우측 정렬
	}

	//VIRT 출력
	if(startCol <= VIRT_IDX && VIRT_IDX < endCol){
		gap = columnWidth[VIRT_IDX] - strlen(VIRT_STR);	//VSZ의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[VIRT_IDX] + gap, "%s", VIRT_STR);	//우측 정렬
	}

	//RES 출력
	if(startCol <= RES_IDX && RES_IDX < endCol){
		gap = columnWidth[RES_IDX] - strlen(RES_STR);	//RSS의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[RES_IDX] + gap, "%s", RES_STR);	//우측 정렬
	}

	//SHR 출력
	if(startCol <= SHR_IDX && SHR_IDX < endCol){
		gap = columnWidth[SHR_IDX] - strlen(SHR_STR);	//SHR의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[SHR_IDX] + gap, "%s", SHR_STR);	//우측 정렬
	}

	//S 출력
	if(startCol <= S_IDX && S_IDX < endCol){
		mvprintw(COLUMN_ROW, startX[S_IDX], "%s", S_STR);	//우측 정렬
	}

	//%CPU 출력
	if(startCol <= CPU_IDX && CPU_IDX < endCol){
		gap = columnWidth[CPU_IDX] - strlen(CPU_STR);	//CPU의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[CPU_IDX] + gap, "%s", CPU_STR);	//우측 정렬
	}

	//%MEM 출력
	if(startCol <= MEM_IDX && MEM_IDX < endCol){
		gap = columnWidth[MEM_IDX] - strlen(MEM_STR);	//MEM의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[MEM_IDX] + gap, "%s", MEM_STR);	//우측 정렬
	}

	//TIME+ 출력
	if(startCol <= TIME_P_IDX && TIME_P_IDX < endCol){
		gap = columnWidth[TIME_P_IDX] - strlen(TIME_P_STR);	//TIME의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[TIME_P_IDX] + gap, "%s", TIME_P_STR);	//우측 정렬
	}

	//COMMAND 출력
	mvprintw(COLUMN_ROW, startX[COMMAND_IDX], "%s", COMMAND_STR);	//좌측 정렬

	attroff(A_REVERSE);

	/*****		column 출력 종료	*****/


	/*****		process 출력 시작	*****/

	char token[TOKEN_LEN];
	memset(token, '\0', TOKEN_LEN);

	for(int i = row; i < procCnt; i++){

		//PID 출력
		if(startCol <= PID_IDX && PID_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", sorted[i]->pid);
			gap = columnWidth[PID_IDX] - strlen(token);	//PID의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[PID_IDX]+gap, "%s", token);	//우측 정렬
		}

		//USER 출력
		if(startCol <= USER_IDX && USER_IDX < endCol){
			gap = columnWidth[USER_IDX] - strlen(sorted[i]->user);	//TIME의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[USER_IDX], "%s", sorted[i]->user);	//좌측 정렬
		}

		//PR 출력
		if(startCol <= PR_IDX && PR_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%d", sorted[i]->priority);
			gap = columnWidth[PR_IDX] - strlen(token);	//PR의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[PR_IDX]+gap, "%s", token);	//우측 정렬
		}

		//NI 출력
		if(startCol <= NI_IDX && NI_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%d", sorted[i]->nice);
			gap = columnWidth[NI_IDX] - strlen(token);	//NI의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[NI_IDX]+gap, "%s", token);	//우측 정렬
		}

		//VIRT 출력
		if(startCol <= VIRT_IDX && VIRT_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", sorted[i]->vsz);
			gap = columnWidth[VIRT_IDX] - strlen(token);	//VIRT의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[VIRT_IDX]+gap, "%s", token);	//우측 정렬
		}

		//RES 출력
		if(startCol <= RES_IDX && RES_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", sorted[i]->rss);
			gap = columnWidth[RES_IDX] - strlen(token);	//RES의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[RES_IDX]+gap, "%s", token);	//우측 정렬
		}

		//SHR 출력
		if(startCol <= SHR_IDX && SHR_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", sorted[i]->shr);
			gap = columnWidth[SHR_IDX] - strlen(token);	//SHR의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[SHR_IDX]+gap, "%s", token);	//우측 정렬
		}

		//S 출력
		if(startCol <= S_IDX && S_IDX < endCol){
			gap = columnWidth[S_IDX] - strlen(sorted[i]->stat);	//S의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[S_IDX], "%s", sorted[i]->stat);	//좌측 정렬
		}

		//%CPU 출력
		if(startCol <= CPU_IDX && CPU_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%3.1Lf", sorted[i]->cpu);
			gap = columnWidth[CPU_IDX] - strlen(token);	//CPU의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[CPU_IDX]+gap, "%s", token);	//우측 정렬
		}

		//%MEM 출력
		if(startCol <= MEM_IDX && MEM_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%3.1Lf", sorted[i]->mem);
			gap = columnWidth[MEM_IDX] - strlen(token);	//MEM의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[MEM_IDX]+gap, "%s", token);	//우측 정렬
		}

		//TIME+ 출력
		if(startCol <= TIME_P_IDX && TIME_P_IDX < endCol){
			gap = columnWidth[TIME_P_IDX] - strlen(sorted[i]->time);	//TIME의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[TIME_P_IDX]+gap, "%s", sorted[i]->time);	//우측 정렬
		}

		//COMMAND 출력
		int tap = col - COMMAND_IDX;
		if((col == COMMAND_IDX) && (strlen(sorted[i]->command) < tap*TAB_WIDTH))		//COMMAND를 출력할 수 없는 경우
			continue;
		if(col < COLUMN_CNT - 1)	//다른 column도 함께 출력하는 경우
			tap = 0;
		sorted[i]->cmd[maxCmd] = '\0';
		mvprintw(COLUMN_ROW+1+i-row, startX[COMMAND_IDX], "%s", sorted[i]->cmd + tap*TAB_WIDTH);	//좌측 정렬

	}

	/*****		process 출력 종료	*****/

	return;
}

int main(int argc, char *argv[])
{
	memTotal = get_mem_total();					//전체 물리 메모리 크기
	hertz = (unsigned int)sysconf(_SC_CLK_TCK);	//os의 hertz값 얻기(초당 context switching 횟수) 시간을 체크하기 위한 값이다.
	now = time(NULL); //현재 시간을 저장한다.

	memset(cpuTimeTable, (unsigned long)0, PID_MAX); // pid의 최대 개수 ( 32768 만큼 cpuTimeTable 배열을 초기화 한다)

	myPid = getpid();			//자기 자신의 pid

	char pidPath[FNAME_LEN];	 //pid 경로를 저장할 문자열
	memset(pidPath, '\0', FNAME_LEN);
	sprintf(pidPath, "/%d", myPid);

	strcpy(myPath, PROC);			//자기 자신의 /proc 경로 획득한다.
	strcat(myPath, pidPath);

	getTTY(myPath, myTTY);			//자기 자신의 tty 획득 tty는 컴퓨터 소통을 위한 가상 콘솔

	//myTTY 배열에서, 숫자가 아닌 값이 나오면 null문자로 바꾸어줌.
	for(int i = strlen(PTS); i < strlen(myTTY); i++)
		if(!isdigit(myTTY[i])){
			myTTY[i] = '\0';
			break;
		}

	myUid = getuid();			//자기 자신의 uid를 불러들인다.

	initscr();				//출력 윈도우 초기화
	halfdelay(10);			//0.1초마다 입력 새로 갱신
	noecho();				//echo 제거
	keypad(stdscr, TRUE);	//특수 키 입력 허용
	curs_set(0);			//curser invisible

	search_proc(false, false, false, false, cpuTimeTable); // /proc 디렉토리를 탐색하는 함수이다. 
					//앞의 4개의 인자는 option 을 부여할때 사용된다. 현재는 사용하지 않았다.

	row = 0;
	col = 0;

	ch = 0;

	bool print = false;  //방향키 입력했을 경우에 true 인 bool형 인수
	pid_t pid; //PID를 저장하는 타입의 구조체 선언

	before = time(NULL); // 3초마다 갱신을 위한 시간 설정

	sort_by_cpu();			//cpu 순으로 정렬
	print_mytop();			//초기 출력
	refresh();				//화면 갱신

	do{						//무한 반복
		now = time(NULL);	//현재 시각 갱신

		switch(ch){			//방향키 입력 좌표 처리
			case KEY_LEFT:
				col--;
				if(col < 0)		//더 왼쪽으로 못가도록
					col = 0;
				print = true;
				break;
			case KEY_RIGHT:
				col++;
				print = true;
				break;
			case KEY_UP:
				row--;
				if(row < 0)
					row = 0;
				print = true;
				break;
			case KEY_DOWN:
				row++;
				if(row > procCnt)
					row = procCnt;
				print = true;
				break;
		}

		if(print || now - before >= 3){	//방향키를 눌렀거나, 3초 경과 시 화면 갱신
			erase();
			erase_proc_list(); // procList 내용 지움
			search_proc(false, false, false, false, cpuTimeTable); //proc 디렉터리 재탐색
			sort_by_cpu();			//cpu 순으로 정렬
			print_mytop();			//다시 mytop 프린트
			refresh();
			before = now;			//시간 갱신
			print = false;			//while문을 다시 돌기 위해 print = false
		}

	}while((ch = getch()) != 'q');	//q 입력 시 종료

	endwin();

	return 0;
}
