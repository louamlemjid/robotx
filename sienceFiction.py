import mediapipe as mp
import cv2
import numpy as np
import os
import uuid
import time
import math as m
import pyautogui

screenWidth, screenHeight = pyautogui.size()

widthRatio,heightRatio=screenWidth/600,screenHeight/400

print(screenWidth, screenHeight)
mp_drawing=mp.solutions.drawing_utils
mp_hands=mp.solutions.hands

cap=cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH,600)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT,400)
cTime=0
pTime=0
with mp_hands.Hands(min_detection_confidence=0.8,min_tracking_confidence=0.5) as hands:
    while cap.isOpened():
        ret,frame=cap.read()
        #detection
        image=cv2.cvtColor(frame,cv2.COLOR_BGR2RGB)
        # Flip on horizontal
        image = cv2.flip(image, 1)
        # Set flag
        image.flags.writeable = False
        # Detections
        results = hands.process(image)
        # Set flag to true
        image.flags.writeable = True
#to bgr
        image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
        
        if results.multi_hand_landmarks!= None:
            coor=results.multi_hand_world_landmarks[0]
            
        if results.multi_hand_landmarks:
            for num, hand in enumerate(results.multi_hand_landmarks):
                for id,lm in enumerate(hand.landmark):
                    # print(id,lm)
                    if id==0:
                        x0=lm.x
                        y0=lm.y
                    if id==4:
                        x4=lm.x
                        y4=lm.y
                    if id==5:
                        x5=lm.x
                        y5=lm.y
                    # if id==2:
                    #     x2=lm.x*100
                    #     y2=lm.y*100
                    if id==17:
                        x17=lm.x
                        y17=lm.y
                    if id==8:
                        x8=lm.x
                        y8=lm.y
                
                print(m.sqrt((x8-x5)*(x8-x5)+(y8-y5)*(y8-y5))*100)
                mp_drawing.draw_landmarks(image, hand, mp_hands.HAND_CONNECTIONS, 
                                        mp_drawing.DrawingSpec(color=(121, 22, 76), thickness=2, circle_radius=4),
                                        mp_drawing.DrawingSpec(color=(250, 44, 0), thickness=2, circle_radius=2),
                                         )
                pyautogui.moveTo(x8*600*widthRatio,y8*heightRatio*400)
                distance48=int(m.sqrt((x8-x4)*(x8-x4)+(y8-y4)*(y8-y4))*100)
                ratio170=m.sqrt((x17-x0)*(x17-x0)+(y17-y0)*(y17-y0))
                
                x48=(x4+x8)*400/2
                y48=(y4+y8)*400/2
                if int((1-ratio170)*distance48) <=2:
                    pyautogui.click()
                cv2.putText(image,str(int((1-ratio170)*distance48)),(int(x48)+110,int(y48)+20),cv2.FONT_HERSHEY_COMPLEX_SMALL,
                            1,(250,250,250),2)
        cTime=time.time()
        fps=1/(cTime-pTime)
        pTime=cTime

        cv2.putText(image,"fps: "+str(int(fps)),(10,30),cv2.FONT_HERSHEY_COMPLEX,1,(200,200,200),2)
        
        cv2.imshow("hand track",image)
        if cv2.waitKey(10) &  0xFF == ord('q'):
            break

cap.release()
cv2.destroyAllWindows()

