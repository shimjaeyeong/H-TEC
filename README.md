# H-TEC
(High Temperature Entrance Checking Technique)

## 사용 센서
적외선 온도 센서  
인체 감지 센서  
피에조  
dot-matrix  
서보모터  

## 시나리오
사람이 온다 – 인체감지센서  
온도를 측정한다 – 적외선온도센서  
if 온도가 37 이상 {<br>
&nbsp;&nbsp;&nbsp;&nbsp;경고 – 피에조, dot-matrix    
} else {<br>
&nbsp;&nbsp;&nbsp;&nbsp;알림 – dot-matrix    
&nbsp;&nbsp;&nbsp;&nbsp;문 열림 – 서보 모터    
}    


## 참고 자료
- Datasheet - STM32D102x8 STM32F102xB - https://www.st.com/resource/en/datasheet/stm32f102c8.pdf
- JK전자와 함께하는 ARM 완전정복(6)-2 - http://www.ntrexgo.com/archives/21271
- PWM을 이용한 서보 모터 작동 - https://lifeseed.tistory.com/81
- PWM with buzzer - https://community.st.com/s/question/0D50X00009Xkanh/pwm-with-buzzer