import os
import cv2

for img in os.listdir("./"):
    if(img[-4:] == ".jpg"):
        image = cv2.imread(img)
        cv2.imwrite(img[:-3]+"png", image)
        os.remove(img)
