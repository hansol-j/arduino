/***********************************************************************************/
/* File Name     : term project.ino                                                */
/* Date          : 2017/6/9                                                        */
/* Compiler      : 아두이노 1.8.1                                                  */
/* Author        : 정한솔                                                          */
/* Student ID    : 12123950                                                        */
/***********************************************************************************/
/*  6조 term project - LED 매트릭스를 이용한 자동차 피하기 게임                     */
/*      matrix 좌표         
 *   0 1 2 3 4 5 6 7           dir
 *   1                        7 0 1
 *   2                        6   2
 *   3                        5 4 3
 *   4
 *   5
 *   6
 *   7
 */
 /*
  * 동작순서
  * 1. 아두이노 시작시 스마일 그림이 표시된다.
  * 2. 일정시간이 지나면 게임시작화면인 집모양 그림이 표시된다.
  * 3. 집모양 그림이 표시되는 상태에서 조이스틱을 움직이면 게임을 시작한다.
  * 4. 차와 에너미가 부딪히면 우는 그림이 나오면서 게임이 끝난다.
  * 5. 다음으로 내 차가 피한 에너미 전체 수가 LED 매트릭스에 표시된다.
  * 6. 다시 게임시작화면인 집 모양 그림으로 돌아간다.
  */

#include "LedControl.h"  //8*8 LED 매트릭스 라이브러리
#include "TimerObject.h" //타이머 라이브러리

LedControl lc=LedControl(12,11,10,1); //(DIN, CLK, CS, 주소)
TimerObject *fps = new TimerObject(1/30); //타이머 설정

//변수 선언
unsigned long deadtime = millis();
bool isSmile = true;
bool isHome = true;
bool joyinput = false;
int carY = 3;
float gamespeed = 0.008;
float enemyX[2] = {0, -5}; //에너미 위치 설정
int enemyY[2] = {2, 5};
int index = 0;


//출력할 도트 그림 2진수로 선언
byte smile[] = { //스마일 그림 배열
  B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10100101,
  B10011001,
  B01000010,
  B00111100  
};
byte loose[] = { //차와 에너미가 부딪혔을때 나오는 우는 그림 배열
  B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10011001,
  B10100101,
  B01000010,
  B00111100  
};

byte Home[] = { //게임 시작화면인 집 모양 그림 배열
  B00011000,
  B00100100,
  B01000010,
  B11111111,
  B01000010,
  B01011010,
  B01011010,
  B01000010
};
byte number[][8] = {  //게임점수 표현하기 위한 숫자 배열
  { //0
    B00000110,
    B00001001,
    B00001001,
    B00001001,
    B00001001,
    B00001001,
    B00001001,
    B00000110
  },
  { //1
    B00000010,
    B00000110,
    B00000010,
    B00000010,
    B00000010,
    B00000010,
    B00000010,
    B00000010
  },
  { //2
    B00000110,
    B00001001,
    B00000001,
    B00000010,
    B00000100,
    B00001000,
    B00001000,
    B00001111
  },
  { //3
    B00000110,
    B00001001,
    B00000001,
    B00000110,
    B00000001,
    B00000001,
    B00001001,
    B00000110
  },
  { //4
    B00000001,
    B00000011,
    B00000101,
    B00001001,
    B00001111,
    B00000001,
    B00000001,
    B00000001
  },
  { //5
    B00001111,
    B00001000,
    B00001000,
    B00001110,
    B00000001,
    B00000001,
    B00001001,
    B00000110
  },
  { //6
    B00000110,
    B00001000,
    B00001000,
    B00001110,
    B00001001,
    B00001001,
    B00001001,
    B00000110
  },
  { //7
    B00001111,
    B00001001,
    B00001001,
    B00000001,
    B00000001,
    B00000001,
    B00000001,
    B00000001
  },
  { //8
    B00000110,
    B00001001,
    B00001001,
    B00000110,
    B00001001,
    B00001001,
    B00001001,
    B00000110
  },
  { //9
    B00000110,
    B00001001,
    B00001001,
    B00000111,
    B00000001,
    B00000001,
    B00000001,
    B00000110
  }
};

void setup()
{
  Serial.begin(9600);
  lc.shutdown(0, false); //LED 매트릭스 절전모드 끄기 
  lc.setIntensity(0,5); //LED 매트릭스 밝기 조절
  lc.clearDisplay(0);   //LED 매트릭스 리셋
  fps->setOnTimer(&gameUpdate); //타이머를 이용해 thread를 실행
  fps->Start();  //thread 실행
}

void printNumber(int num) //게임점수를 2자리 숫자로 LED 매트릭스에 출력할 수 있다.
{
  int first = num / 10;
  int second = num % 10;
  
  for (int i = 0; i < 8; ++i) {
    lc.setRow(0,i,(first > 0 ? (number[first][i] << 4) : 0) + (number[second][i]));
  }
}

//미리 선언해둔 도트 그림을 출력하는 함수
void displayDot(byte *row){ 
  for(int i = 0; i<8; i++)
    lc.setRow(0, i, row[i]);
}

//조이스틱의 움직임을 읽어 차의 움직임을 결정
void carDirection(int dir){ //dir>0 이면 오른쪽 dir<0이면 왼쪽으로 이동
  if(dir>0 && carY<6)
    carY += 1;
  if(dir<0 && carY>1)
    carY -= 1;
}

/*
 *     *
 *    ***
 */
//car를 표시한다.
void displayCar(){
  lc.setLed(0, 7, carY, true);
  lc.setLed(0, 6, carY, true);
  lc.setLed(0, 7, carY-1, true);
  lc.setLed(0, 7, carY+1, true);
}

//에너미의 움직임을 업데이트한다.
void updatePos(){
  for(int i = 0; i<2; i++){
    enemyX[i] += gamespeed; 
    if(enemyX[i] > 9){  //에너미가 차를 완전히 지나쳐가면 새로운 에너미를 불러온다.
      if(((0<enemyX[1])&&(enemyX[1]<4))||((enemyX[2]<4)&&(0<enemyX[2]))){ //에너미가 작은 거리차이로 생성되어 피할수 없게 되는 상황을 없앤다. 
        enemyX[i] = -random(4,6), index++; //랜덤함수를 이용하여 에너미 생성에 랜덤요소를 주었다.
        if(random(2))                      //index로 에너미 생성한 수를 파악해 차가 에너미를 피한 숫자를 계산
         enemyY[i] = 2;
       else
         enemyY[i] = 5;
      } else if((enemyX[1]>=4) || (enemyX[2]>=4)){
        enemyX[i] = -random(1,4), index++;
       if(random(2))
         enemyY[i] = 2;
       else
         enemyY[i] = 5;
      }
      else{
        enemyX[i] = -random(6,8), index++;
       if(random(2))
         enemyY[i] = 2;
       else
         enemyY[i] = 5;
      }
    }
  }
}

void restartGame(){ //게임이 다시 시작되면 변수 초기화
  carY = 3;
  enemyY[0] = 2;
  enemyY[1] = 5;
  enemyX[0] = 0;
  enemyX[1] = -5;
}

void carCrash(){ //차와 에너미가 충돌하면 우는 표정을 출력하고 게임을 리스타트한다.
  deadtime = millis();
  lc.clearDisplay(0);
  displayDot(loose);
  delay(1000); //게임이 끝났으므로 딜레이는 사용했지만 다른 곳에서는 타이머를 이용하여 딜레이 구현했다.
  lc.clearDisplay(0);
  printNumber(index); //점수를 LED 매트릭스에 출력
  delay(2000);
  index = 0;
  isHome=true;
  restartGame();
}

void checkCarCrash(){ //차가 충돌했는지 안했는지 검사한다.
  for(int i = 0; i < 2; i++){
    int xx = (int)enemyX[i], yy = enemyY[i];
    int gap = abs(carY - yy);
    if(xx >= 8 && gap <=2)
      carCrash();
  }
}

void displayEnemy(){ //에너미의 현재 위치를 LED 매트릭스에 표시한다.
  for(int i = 0; i<2; i++){
    int ex = (int) enemyX[i];
    int ey = (int) enemyY[i];

    if(ex>=0 && ex<= 7)
      lc.setLed(0, ex, ey, true);
    if(ey-1>=0 && ey -1<= 7){
      lc.setLed(0, ex-1, ey, true);
      lc.setLed(0, ex-1, ey -1, true);
      lc.setLed(0, ex-1, ey +1, true);
    }
  }
}

void gameUpdate(){ //타이머를 이용해 계속 실행되게 만든 게임thread이다.
  updatePos();
  checkCarCrash();
  lc.clearDisplay(0);
  displayCar();
  displayEnemy();
}


//타이머를 이용하여 게임스레드가 계속 동작하게 하고
//루프 함수에서는 조이스틱의 움직임을 감지해 게임을 시작하고
//차의 움직임 방향을 결정한다. 
void loop()
{
  int dir = 0; 
  int val = analogRead(0); //아날로그A0포트를 통해 조이스틱 움직임 감지
  if(val<350) //조이스틱 움직임에 따라 방향 결정
    dir = -1;
  else if(val>750)
    dir = 1;

  if(isSmile){ //아두이노 시작시 처음 나오는 스마일그림
    lc.clearDisplay(0);
    displayDot(smile);

  if(millis() - deadtime > 3000) //일정 시간이 지나면 스마일 그림은 없앤다.
    isSmile = false;
  } else{
    if(isHome){ //일정 시간이 지나면 게임시작화면인 집모양의 그림이 출력된다.
      lc.clearDisplay(0);
      displayDot(Home);
      if(dir != 0 && millis() - deadtime>1000) //집모양의 그림이 있는 상태에서 조이스틱의 움직임이 감지되면 게임이 시작된다.
        isHome = false;
    }else{
      if(!joyinput) //조이스틱의 움직에 따라 결정된 방향 전달
        carDirection(dir);
      fps->Update(); //타이머를 체크해 필요하면  함수 콜백
    }

    if(dir == 0){ //조이스틱 움직임 있는지 없는지 검사, 집모양 그림일때 움직임이 있으면 게임시작
      joyinput = false;
    } else{
      joyinput = true;
    }
  }
}

