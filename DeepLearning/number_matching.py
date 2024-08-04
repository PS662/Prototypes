from PIL import Image
import numpy as np
import matplotlib.pyplot as plt
from collections import Counter
from matplotlib import style
from statistics import mean

style.use("ggplot")

def createExamples():
    # Updated file handling to automatically close the file
    with open('numArEx1.txt', 'a') as numberArrayExamples:
        numbersWeHave = range(1, 10)
        for eachNum in numbersWeHave:
            for furtherNum in numbersWeHave:
                imgFilePath = f'images/numbers/{eachNum}.{furtherNum}.png'
                ei = Image.open(imgFilePath)
                eiar = np.array(ei)
                eiarl = eiar.tolist()

                lineToWrite = f'{eachNum}::{eiarl}\n'
                numberArrayExamples.write(lineToWrite)

def threshold(imageArray):
    balanceAr = [mean(pixel[:3]) for row in imageArray for pixel in row]
    balance = mean(balanceAr)
    newAr = np.where(mean(imageArray[:, :, :3], axis=2) > balance, 255, 0)
    return np.stack([newAr] * 4, axis=-1)  # Stack 4 times along the last axis for RGBA

def whatNumIsThis(filePath):
    matchedAr = []
    with open('numArEx.txt', 'r') as file:
        loadExamps = file.read().split('\n')

    i = Image.open(filePath)
    iar = np.array(i)
    iarl = iar.tolist()
    inQuestion = str(iarl)

    for eachExample in loadExamps:
        try:
            splitEx = eachExample.split('::')
            currentNum, currentAr = splitEx
            eachPixEx = currentAr.split('],')
            eachPixInQ = inQuestion.split('],')

            for x, (pixEx, pixInQ) in enumerate(zip(eachPixEx, eachPixInQ)):
                if pixEx == pixInQ:
                    matchedAr.append(int(currentNum))
        except Exception as e:
            print(e)

    x = Counter(matchedAr)
    graphX, graphY = zip(*x.items())

    fig = plt.figure()
    ax1 = plt.subplot2grid((4,4), (0,0), rowspan=1, colspan=4)
    ax2 = plt.subplot2grid((4,4), (1,0), rowspan=3, colspan=4)
    
    ax1.imshow(iar)
    ax2.bar(graphX, graphY, align='center')
    plt.ylim(0, max(graphY) + 10)
    
    xloc = plt.MaxNLocator(12)
    ax2.xaxis.set_major_locator(xloc)

    plt.show()

whatNumIsThis('images/test2.png')
