#!/usr/bin/env python
# -*- coding: utf-8 -*-


import matplotlib.pyplot as plt
import numpy as np
import yaml
import glob, os
from scipy.integrate import odeint

# master equations
def sird(X, t):
     S = X[0]
     J = X[1]
     f0 = - alpha*S*J
     f1 =   alpha*S*J - beta*J - delta*J
     f2 =   beta* J
     f3 =   delta*J
     return [f0, f1, f2, f3]

# plotter 
def plotter(S, J, R, D, Time, file, rare):
    fig, ax = plt.subplots(figsize=(16, 10), dpi=150, facecolor='w', edgecolor='k')
    ax.set_yscale('log')
    #ax.set_ylim([0,15000])
    #ax.plot(Time, S, '-', color='blue',  linewidth=3, label='Healthy')
    ax.plot(Time, J, '-', color='red',   linewidth=3, label='Ill')
    #ax.plot(Time, 3.*np.exp((1./21)*Time), '--', color='blue',   linewidth=3, label='Test-exp')
    #ax.plot(Time, R, '-', color='green', linewidth=3, label='Recovered')
    #ax.plot(Time, D, '-', color='black', linewidth=3, label='Dead')
    ax.set_xlabel('Time, in days', fontsize=16)
    ax.set_ylabel('S, J, R, D', fontsize=16)
    ax.set_title('Test COVID-19 model', fontsize=16)
    ax.legend(fontsize=16)
    plt.tick_params(axis='both', which='major', labelsize=14)
    # ax.grid(True)
    
    data = np.genfromtxt(file, delimiter=';')
    ME = np.mean(data[1:,:-1],1) 
    STD = np.std(data[1:,:-1],1)
    DAYS = np.arange(1,len(ME)+1)
    ax.plot(DAYS, data[1:, 1:len(data[1,:-1]):rare], '.', color='tab:blue', markersize=1)
    ax.fill_between(DAYS, ME - 3*STD, ME + 3*STD, alpha=0.2)
    ax.plot(DAYS, ME, '--', color='red', linewidth=5)
    ax.set_xlabel('Time, in days', fontsize=16)
    ax.set_ylabel('Ill', fontsize=16)
    ax.set_title('Mean and STD', fontsize=16)
    plt.tick_params(axis='both', which='major', labelsize=14)
    ax.grid(True)
    # plt.show()
    name = file.split('.')[0] + '.jpg'
    fig.savefig(name)
    plt.show()
    # name = 'ode_solution.jpg'
    fig.savefig(name)

def main(params):
    init_data = [params['popul']['healthy'],
                 params['popul']['injured'],
                 params['popul']['dead'],
                 params['popul']['recovered']]
    Time = np.linspace(0, params['times']['total'], params['times']['n_steps'])
    solution = odeint(sird, init_data, Time)
    S = solution[:,0]
    J = solution[:,1]
    R = solution[:,2]
    D = solution[:,3]
    # co_main()
    os.chdir("./")
    # ищет csv и строит графики для всех найденных
    files = glob.glob("*.csv")
    # первый аргумент - имя файла в текущей директории
    # второй аргумент - шаг прореживания реализций случайных функций при выводе на график
    for file in files:
        print(file)
        plotter(S, J, R, D, Time, file, 1)

if __name__ == '__main__':
    # load the model parameters
    params = yaml.load(open('params.yaml','r'))
    # three global constants      
    
    # injuring rate
    alpha = params['prob']['injure'] / params['popul']['total'] / params['times']['disease']
    # recovering rate
    beta  = params['prob']['recov'] / params['times']['disease']          
    # death rate         
    delta = params['prob']['death'] / params['times']['disease']       
    main(params)
