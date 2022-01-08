#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define LEN 100001
int page[31];

char str1[LEN];
char str2[LEN];

int F_N = 0;
int R_N = 0;

int Frame[4];
int Times = 0;
int Page_Fault[31];
int Fault_Times = 0;
char PageRefence[LEN];
bool bit[5];

int tmp[4];
void printOnce()
{
	printf("%d\t\t", Times + 1);
	for (int i = 0; i < F_N; i++)
	{
		if (Frame[i] == -1)
			printf("\t");
		else
			printf("%d\t", Frame[i]);
	}
	if (Page_Fault[Times])
	{
		printf("F");
		Fault_Times++;
	}
	printf("\n");
	Times++;
}
void HeadPrint()
{
	printf("Page reference string : ");
	printf("%s\n", PageRefence);
	printf("\tframe\t");
	for (int i = 1; i <= F_N; i++)
		printf("%d\t", i);
	printf("page fault\n");
	printf("time\n");
}
void OPT()
{
	HeadPrint();
	int flag = 0;
	for (int i = 0; i < R_N; i++) //매줄
	{
		memset(tmp, 0, sizeof(tmp));
		int num = page[i]; //바꿀친구
		int exist = 0;
		for (int j = 0; j < F_N; j++) //페이지fault 체크
		{
			if (Frame[j] == num)
			{
				exist = 1;
				break;
			}
		}
		if (exist)
		{
			exist = 0;
			printOnce();
			continue;
		}
		Page_Fault[i] = 1;
		for (int j = 0; j < F_N; j++)
		{
			if (Frame[j] == -1) // 아직 암것도 안들어갔을때
			{
				Frame[j] = num;
				flag = 1;
				break;
			}

			if (i != (R_N - 1))				  //맨끝경우
				for (int k = i; k < R_N; k++) // 뒤에 몇번나오는지 기록하기
				{
					if (page[k + 1] == Frame[j]) //겹치면
					{
						tmp[j] = k; // tmp[j] == j번째 테이블에 있는 값이 언제 처음 나오는지
						break;
					}
				}
			if (tmp[j] == 0) // 안나오면
			{
				// printf("%d번째 frame은 뒤에 안나옴", j + 1);
				Frame[j] = num;
				flag = 1;
				break;
			}
		}
		if (flag)
		{
			flag = 0;
			printOnce();
			continue;
		}

		int changeIndex = 0;
		int max = 0;
		for (int k = 0; k < F_N; k++)
		{
			if (max < tmp[k])
			{ // k번째 프레임이 가장 늦을때
				max = tmp[k];
				changeIndex = k;
			}
		}
		if (i == 10)
			printf("바꿀 프레임 번호 : %d\n", changeIndex + 1);
		Frame[changeIndex] = num;

		printOnce();
	}

	printf("Number of  page faults : %d times\n", Fault_Times);
}

void FIFO()
{
	HeadPrint();
	int flag = 0;
	char *a = NULL;
	int Index = 0;
	for (int i = 0; i < R_N; i++) //매줄
	{
		memset(tmp, 0, sizeof(tmp));
		int num = page[i]; //바꿀친구
		int exist = 0;
		for (int j = 0; j < F_N; j++) //페이지fault 체크
		{
			if (Frame[j] == num)
			{
				exist = 1;
				break;
			}
		}
		if (exist)
		{
			exist = 0;
			printOnce();
			continue;
		}
		Page_Fault[i] = 1;

		if (Index < F_N) // 3보다 작다. 0 1 2
		{
			Frame[Index] = num;
			Index++;
		}
		else
			printf("범위 넘어섬\n");

		if (Index >= F_N)
			Index = 0;
		printOnce();
	}
	printf("Number of  page faults : %d times\n", Fault_Times);
}

void Second_Chance()
{
	HeadPrint();
	int flag = 0;
	char *a = NULL;
	int Index = 0;
	for (int i = 0; i < R_N; i++) //매줄
	{
		memset(tmp, 0, sizeof(tmp));
		int num = page[i]; //바꿀친구
		int exist = 0;
		for (int j = 0; j < F_N; j++) //페이지fault 체크
		{
			if (Frame[j] == num) //있을때
			{
				bit[j] = true;
				exist = 1;
				break;
			}
		}
		if (exist)
		{
			exist = 0;
			printOnce();
			continue;
		}
		Page_Fault[i] = 1;

		while (true)
		{
			if (Index < F_N) // 3보다 작다. 0 1 2
			{
				if (bit[Index])
					bit[Index] = false;
				else
				{
					Frame[Index] = num;
					Index++;
					break;
				}
				Index++;
			}
			// else
			// printf("범위 넘어섬\n");
			if (Index >= F_N)
				Index = 0;
		}
		printOnce();
	}
	printf("Number of  page faults : %d times\n", Fault_Times);
}

void LRU()
{
	HeadPrint();
	int flag = 0;
	for (int i = 0; i < R_N; i++) //매줄
	{
		for (int i = 0; i < F_N; i++)
			tmp[i] = 999;

		int num = page[i]; //바꿀친구
		int exist = 0;
		for (int j = 0; j < F_N; j++) //페이지fault 체크
		{
			if (Frame[j] == num)
			{
				exist = 1;
				break;
			}
		}
		if (exist)
		{
			exist = 0;
			printOnce();
			continue;
		}
		Page_Fault[i] = 1;
		for (int j = 0; j < F_N; j++)
		{
			if (Frame[j] == -1) // 아직 암것도 안들어갔을때
			{
				Frame[j] = num;
				flag = 1;
				break;
			}

			//맨끝경우
			for (int k = i; k > 0; k--) // 뒤에 몇번나오는지 기록하기
			{
				if (page[k - 1] == Frame[j]) //겹치면, and i가 0일땐 올수가없으니 괜찮
				{
					tmp[j] = k; // tmp[j] == j번째 테이블에 있는 값이 언제 처음 나오는지
					break;
				}
			}

			if (tmp[j] == 0) // 안나오면
			{
				// printf("%d번째 frame은 뒤에 안나옴", j + 1);
				Frame[j] = num;
				flag = 1;
				break;
			}
		}
		if (flag)
		{
			flag = 0;
			printOnce();
			continue;
		}

		int changeIndex = 0;
		int min = 99;
		for (int k = 0; k < F_N; k++)
		{
			if (min > tmp[k])
			{ // k번째 프레임이 가장 늦을때
				min = tmp[k];
				changeIndex = k;
			} //세개 다 없으면?
		}
		Frame[changeIndex] = num;
		printOnce();
	}
	printf("Number of  page faults : %d times\n", Fault_Times);
}

int main()
{
	char fname[20];
	printf("%s","Please Put File name : ");
	scanf("%s",fname);
	FILE* fp=fopen(fname,"r");
	if (fp == NULL)
	{
		printf("File Name is wrong.\n");
		exit(1);
	}
	fgets(str1, LEN, fp);
	fgets(str2, LEN, fp);
	strcpy(PageRefence, str2);
	fclose(fp);
	F_N = atoi(&str1[0]);

	if (F_N < 1 || F_N > 4)
	{
		printf("put number between 1 ~ 4\n");
		return -1;
	}

	char *ptr = strtok(str2, " "); // " " 공백 문자를 기준으로 문자열을 자름, 포인터 반환
	while (ptr != NULL)			   // 자른 문자열이 나오지 않을 때까지 반복
	{
		page[R_N] = atoi(&ptr[0]); // 자른 문자열 출력
		R_N++;
		ptr = strtok(NULL, " ");
		if (R_N > 30)
		{
			printf("Reference Num must be 1~30\n");
			return -1;
		}
	}
	if (R_N == 0)
	{
		printf("Reference Num must be 1~30\n");
		return -1;
	}
	memset(Frame, -1, sizeof(Frame));

	printf("Used method (1 : OPT, 2: FIFO, 3:LRU, 4:Second-Chance) : ");
	int button = 0;
	scanf("%d", &button);
	switch (button)
	{
	case 1:
		OPT();
		break;
	case 2:
		FIFO();
		break;
	case 3:
		LRU();
		break;
	case 4:
		Second_Chance();
		break;
	default:
		printf("put 1~4\n");
	}
}
