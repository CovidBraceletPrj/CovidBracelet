import json
import os

import numpy as np
import math

import matplotlib.pyplot as plt
from eval_utility import slugify, cached, init_cache, load_env_config

import matplotlib.ticker as ticker
from matplotlib.ticker import PercentFormatter
from matplotlib.ticker import (MultipleLocator, AutoMinorLocator)
import matplotlib as mpl

VOLTS = 3.0

METHOD_PREFIX = 'export_'

CONFIDENCE_FILL_COLOR = '0.8'

NUM_NODES = 24

def load_plot_defaults():
    # Configure as needed
    plt.rc('lines', linewidth=2.0)
    plt.rc('legend', framealpha=1.0, fancybox=True)
    plt.rc('errorbar', capsize=3)
    plt.rc('pdf', fonttype=42)
    plt.rc('ps', fonttype=42)
    plt.rc('font', size=8, family="serif", serif=['Times New Roman'] + plt.rcParams['font.serif'])
    #mpl.style.use('tableau-colorblind10')


idle_avg = 0.00252
idle_max = 0.02325

IDLE_LABEL = 'idle'
consumptions = {}
durations = {}
times_per_day = {}
scaled_consumptions = {}
raw_scaled_consumptions = {}

def add_consumption(label, consumption_msrmnt, duration, tpd, repetitions=1):
    consumptions[label] = consumption_msrmnt
    durations[label] = duration/repetitions
    times_per_day[label] = tpd


def calculate_usage_seconds_per_day(labels, normalize_with_idle=True):
    # we first calculate the times for each label
    usage_seconds = {
        IDLE_LABEL: 0
    }
    sum = 0
    for l in labels:
        if l == IDLE_LABEL:
            continue
        usage_seconds[l] = durations[l]*times_per_day[l]
        assert 0 <= usage_seconds[l] <= 24*3600.0
        sum += usage_seconds[l]

    if sum < 24*3600.0:
        usage_seconds[IDLE_LABEL] = 24*3600.0-sum   # we spent the rest of the time idling!

    cons_per_day = {}
    for l in labels:
        cons_per_day[l] = usage_seconds[l]   # also convert to ampere hours!
    return cons_per_day

def calculate_consumption_per_day(labels, normalize_with_idle=True):
    # we first calculate the times for each label
    usage_seconds = {
        IDLE_LABEL: 0
    }
    sum = 0
    for l in labels:
        if l == IDLE_LABEL:
            continue
        usage_seconds[l] = durations[l]*times_per_day[l]
        assert 0 <= usage_seconds[l] <= 24*3600.0
        sum += usage_seconds[l]

    if sum < 24*3600.0:
        usage_seconds[IDLE_LABEL] = 24*3600.0-sum   # we spent the rest of the time idling!

    cons_per_day = {}
    for l in labels:
        cons_per_day[l] = consumptions[l]*usage_seconds[l]*(1/3600.0)   # also convert to ampere hours!
    return cons_per_day


    # calculate the remaining idle time



# THE TOTAL EXPECTED AMOUNT PER DAY in milli ampere
expected_consumption_per_day = 0.0

idle_consumption = 0.00256
add_consumption(IDLE_LABEL, idle_consumption, 1.0, 1.0)

# ADVERTISING
adv_interval = 0.250
adv_consumptions = [
    2.45,
    2.47,
    2.41,
    2.35,
    2.47,
    2.39,
    2.45,
    2.47,
    2.49,
    2.45
]

adv_max_consumption = [
    7.66,
    8.31,
    6.96,
    7.3,
    7.77,
    7.6,
    8.55,
    6.94,
    7.14,
    6.85
]



# TODO: Add error bars if possible!
# measured_duration
# duration
# repetitions
adv_consumption = sum(list(adv_consumptions)) / len(adv_consumptions)

add_consumption('adv', adv_consumption, 0.004, (24*3600)/0.25)

# SCANNING
scan_consumption = 6.01
scan_consumption_max = 8.71
add_consumption('scan', scan_consumption, 2.015, 24*12)


crypt_duration = 0.22
crypt_consumption_avg = 3.2
crypt_consumption_max = 5.96

add_consumption('daily_crypto', crypt_consumption_avg, 0.22, 10)

# A table for the timings of the cryptographic fundamentals
# One detailed graph as a comparison of the advertisements
# One bar graph for each of the factors involved in the daily energy usage

tek_check_duration = 2.081
tek_check_amount = 144
tek_check_consumption = 3.74
tek_check_consumption_max = 4.49


# generate graph with keys to check
# Worst case scenario: Flash is fully used!
# we generate a bloom filter of all records!


# How long does it take to create the bloom filter for the whole dataset?
#64kByte



# then check keys (i.e., maybe 32 teks at once?)
# measure time
# measure consumption
# extrapolate numbers for more keys!


def export_consumption_per_day():
    cpd = calculate_consumption_per_day([
        IDLE_LABEL, 'adv', 'scan', 'daily_crypto'
    ])
    print("export_consumption_per_day")
    print(cpd)
    print(sum(cpd.values()))
    ys = ['Idle', 'Adv.', 'Scan', 'Crypto\n(Daily)']
    xs = [cpd[IDLE_LABEL], cpd['adv'], cpd['scan'], cpd['daily_crypto']]

    fig, ax = plt.subplots()

    ax.set_ylabel('Avg. Consumption per Day [mA h]')
    ax.set_xlabel('Functionality')

    bars = ax.bar(ys,xs)

    xs_labels = ["{:.2f}".format(x) if x >= 0.01 else "<0.01" for x in xs]
    ax.bar_label(bars, padding=3, labels=xs_labels)

    # Adapt the figure size as needed

    fig.set_size_inches(3.0, 3.2)
    ax.set_ylim([0, 2])
    plt.tight_layout()
    plt.savefig("../out/weighted_consumption.pdf", format="pdf", bbox_inches='tight')
    plt.close()


def export_usage_seconds_per_day():
    cpd = calculate_usage_seconds_per_day([
        IDLE_LABEL, 'adv', 'scan', 'daily_crypto'
    ])
    print("export_usage_seconds_per_day")
    print(sum(cpd.values()))
    print(cpd)
    ys = ['Idle', 'Adv.', 'Scan', 'Crypto\n(Daily)']
    xs = [cpd[IDLE_LABEL], cpd['adv'], cpd['scan'], cpd['daily_crypto']]
    xs = [100*x/(24*3600) for x in xs]

    fig, ax = plt.subplots()

    ax.set_ylabel('Estimated Duration per Day [%]')
    ax.set_xlabel('Functionality')

    bars = ax.bar(ys,xs)

    xs_labels = ["{:.2f}%".format(x) if x >= 0.01 else "<0.01%" for x in xs]
    ax.bar_label(bars, padding=3, labels=xs_labels)

    ax = plt.gca()
    #ax.set_xlim([xmin, xmax])
    ax.set_ylim([0, 109])

    # Adapt the figure size as needed
    fig.set_size_inches(3.0, 3.2)
    plt.tight_layout()
    plt.savefig("../out/export_usage_seconds_per_day.pdf", format="pdf", bbox_inches='tight')
    plt.close()

def export_current_per_functionality():

    ys = ['Idle', 'Adv.', 'Scan', 'Crypto\n(Daily)']
    xs = [consumptions[IDLE_LABEL], consumptions['adv'], consumptions['scan'], consumptions['daily_crypto']]

    print("export_current_per_functionality")
    print(consumptions)

    fig, ax = plt.subplots()

    ax.set_ylabel('Avg. Consumption [mA]')
    ax.set_xlabel('Functionality')

    bars = ax.bar(ys,xs)

    xs_labels = ["{:.2f}".format(x) if x >= 0.01 else "<0.01" for x in xs]
    ax.bar_label(bars, padding=3, fmt='%.2f', labels=xs_labels)

    # Adapt the figure size as needed
    fig.set_size_inches(3.0, 3.2)
    ax.set_ylim([0, 8])
    plt.tight_layout()
    plt.savefig("../out/current_per_functionality.pdf", format="pdf", bbox_inches='tight')
    plt.close()

def export_tek_check():

    xs = [0, 1250000, 2500000, 5000000]

    means = {}

    for l in ['GAEN', 'TEK Transport', 'TEK Check']:
        means[l] = []

    for tpd in xs:
        add_consumption('tek_check_' + str(tpd), tek_check_consumption, tek_check_duration, tpd, repetitions=tek_check_amount)
        add_consumption('tek_transport_' + str(tpd), 8, 1.0, tpd, repetitions=3125)

        print(tpd)

        #upd = calculate_usage_seconds_per_day([
        #    IDLE_LABEL, 'adv', 'scan', 'daily_crypto', 'tek_check_' + str(tpd), 'tek_transport_' + str(tpd)
        #])
        #print(upd)
        cpd = calculate_consumption_per_day([
            IDLE_LABEL, 'adv', 'scan', 'daily_crypto', 'tek_check_' + str(tpd), 'tek_transport_' + str(tpd)
        ])
        print(cpd)

        means['GAEN'].append(cpd[IDLE_LABEL]+ cpd['adv']+cpd['scan']+ cpd['daily_crypto'])
        means['TEK Transport'].append(cpd['tek_transport_' + str(tpd)])
        means['TEK Check'].append(cpd['tek_check_' + str(tpd)])


    labels = ['GAEN', 'TEK Transport', 'TEK Check']

    width = 0.4      # the width of the bars: can also be len(x) sequence

    fig, ax = plt.subplots()


    #xs = [str(x) for x in xs]
    xs = ['0', '1,250,000', '2,500,000', '5,000,000']
    ax.bar(xs, means['GAEN'], width, label='GAEN')
    ax.bar(xs, means['TEK Transport'], width, label='TEK Transport', bottom=means['GAEN'])
    bars = ax.bar(xs, means['TEK Check'], width, label='TEK Check', bottom=[means['GAEN'][i]+means['TEK Transport'][i] for (i,x) in enumerate(xs)])

    bar_labels = [means['GAEN'][i]+means['TEK Transport'][i]+means['TEK Check'][i] for (i,x) in enumerate(xs)]
    bar_labels = ['{:.2f}'.format(l) for l in bar_labels]
    print(bar_labels)
    ax.bar_label(bars, padding=3, labels=bar_labels)

    ax.set_ylabel('Estimated Consumption per Day [mAh]')
    ax.set_xlabel('Number of TEKs per Day')
    ax.legend()

    # Adapt the figure size as needed
    fig.set_size_inches(4.2, 3.5)
    ax.set_ylim([0, 100])
    plt.tight_layout()
    plt.savefig("../out/tek_check.pdf", format="pdf", bbox_inches='tight')
    plt.close()


export_usage_seconds_per_day()
export_consumption_per_day()
export_current_per_functionality()
export_tek_check()