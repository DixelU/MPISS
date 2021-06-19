#!/usr/bin/env python
# -*- coding: utf-8 -*-


import matplotlib.pyplot as plt
import numpy as np
import glob, os


def main(file, rare):
    data = np.genfromtxt(file, delimiter=';')

    ME = np.mean(data[1:,:-1],1)
    STD = np.std(data[1:,:-1],1)
    DAYS = np.arange(1,len(ME)+1)
    fig, ax = plt.subplots(figsize=(16, 10), dpi=150, facecolor='w', edgecolor='k')
    ax.plot(DAYS, data[1:, 1:len(data[1,:-1]):rare], '.', color='tab:blue', markersize=1)
    ax.fill_between(DAYS, ME - 3*STD, ME + 3*STD, alpha=0.2)
    ax.plot(DAYS, ME, '-', color='black', linewidth=3)
    ax.set_xlabel('Time, in days', fontsize=16)
    ax.set_ylabel('Injured', fontsize=16)
    ax.set_title('Mean and STD', fontsize=16)
    plt.tick_params(axis='both', which='major', labelsize=14)
    ax.grid(True)
    # plt.show()
    name = file.split('.')[0] + '.jpg'
    fig.savefig(name)


if __name__ == '__main__':
    # переходит в директорию, куда сохраняются csv
    os.chdir("./")
    # ищет csv и строит графики для всех найденных
    files = glob.glob("*.csv")
    # первый аргумент - имя файла в текущей директории
    # второй аргумент - шаг прореживания реализций случайных функций при выводе на график
    for file in files:
        print(file)
        main(file, 1)
