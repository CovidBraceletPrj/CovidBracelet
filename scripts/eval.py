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

idle_consumption = 0.500
add_consumption(IDLE_LABEL, idle_consumption, 1.0, 1.0)

# ADVERTISING
adv_interval = 0.250
per_adv_consumption = {
    0: 2.23,
    -4: 1.49 ,
    -8: 1.43,
    -16: 1.38,
    -20: 1.3,
    -40: 1.22,
}
# TODO: Add error bars if possible!
# measured_duration
# duration
# repetitions


adv_consumption = sum(list(per_adv_consumption.values())) / len(per_adv_consumption.values())

add_consumption('adv', adv_consumption, 0.01, (24*3600)/0.25)

# SCANNING
scan_consumption = 5.440
add_consumption('scan', scan_consumption, 1.0, 24*60)


crypt_duration = 0.01
generate_tek_consumption = 2
derive_tek_consumption = 2
derive_rpi_consumption = 2
encrypt_aem_consumption = 2

add_consumption('generate_tek', generate_tek_consumption, crypt_duration, 1)
add_consumption('derive_tek', derive_tek_consumption, crypt_duration, 1)
add_consumption('derive_rpi', derive_rpi_consumption, crypt_duration, 144)
add_consumption('encrypt_aem', encrypt_aem_consumption, crypt_duration, 144*len(per_adv_consumption.values()))

# A table for the timings of the cryptographic fundamentals
# One detailed graph as a comparison of the advertisements
# One bar graph for each of the factors involved in the daily energy usage

tek_check_duration = 0.020
tek_check_amount = 32
tek_check_consumption = 4

# generate graph with keys to check
# Worst case scenario: Flash is fully used!
# we generate a bloom filter of all records!


# How long does it take to create the bloom filter for the whole dataset?
#64kByte



# then check keys (i.e., maybe 32 teks at once?)
# measure time
# measure consumption
# extrapolate numbers for more keys!

# We then get keys to check
# 1000, 10000, 100000


def export_adv_consumption():

    xs = [per_adv_consumption[0], per_adv_consumption[-4], per_adv_consumption[-8], per_adv_consumption[-16], per_adv_consumption[-20], per_adv_consumption[-40]]
    ys = ['0', '-4', '-8', '-16', '-20', '-40']

    width = 0.6       # the width of the bars: can also be len(x) sequence

    fig, ax = plt.subplots()

    ax.set_ylabel('Avg. Advertising Current [µA]')
    ax.set_xlabel('TX Power [dBm]')

    bars = ax.bar(ys,xs)
    ax.bar_label(bars, padding=5, fmt='%.2f')

    #ax.set_title('')
    #ax.legend() # (loc='upper center', bbox_to_anchor=(0.5, -0.5), ncol=2)

    # Adapt the figure size as needed
    fig.set_size_inches(3.5, 3.2)
    plt.tight_layout()
    plt.savefig("../out/adv_consumption.pdf", format="pdf", bbox_inches='tight')
    plt.close()


def export_consumption_per_day():
    cpd = calculate_consumption_per_day([
        IDLE_LABEL, 'adv', 'scan', 'generate_tek', 'derive_tek', 'derive_rpi', 'encrypt_aem'
    ])
    print(sum(cpd.values()))
    ys = ['Idle', 'Adv.', 'Scan', 'Crypto\n(Daily)']
    xs = [cpd[IDLE_LABEL], cpd['adv'], cpd['scan'], cpd['generate_tek']+cpd['derive_tek']+cpd['derive_rpi']+ cpd['encrypt_aem']]

    fig, ax = plt.subplots()

    ax.set_ylabel('Avg. Consumption per Day [mA h]')
    ax.set_xlabel('Functionality')

    bars = ax.bar(ys,xs)
    ax.bar_label(bars, padding=5, fmt='%.2f')

    # Adapt the figure size as needed
    fig.set_size_inches(3.5, 3.2)
    plt.tight_layout()
    plt.savefig("../out/weighted_consumption.pdf", format="pdf", bbox_inches='tight')
    plt.close()


def export_usage_seconds_per_day():
    cpd = calculate_usage_seconds_per_day([
        IDLE_LABEL, 'adv', 'scan', 'generate_tek', 'derive_tek', 'derive_rpi', 'encrypt_aem'
    ])
    print(sum(cpd.values()))
    ys = ['Idle', 'Adv.', 'Scan', 'Crypto\n(Daily)']
    xs = [cpd[IDLE_LABEL], cpd['adv'], cpd['scan'], cpd['generate_tek']+cpd['derive_tek']+cpd['derive_rpi']+ cpd['encrypt_aem']]
    xs = [x/(24*3600) for x in xs]

    fig, ax = plt.subplots()

    ax.set_ylabel('Estimated Duration per Day [%]')
    ax.set_xlabel('Functionality')

    bars = ax.bar(ys,xs)
    ax.bar_label(bars, padding=5, fmt='%.2f')

    # Adapt the figure size as needed
    fig.set_size_inches(3.5, 3.2)
    plt.tight_layout()
    plt.savefig("../out/export_usage_seconds_per_day.pdf", format="pdf", bbox_inches='tight')
    plt.close()

def export_current_per_functionality():

    ys = ['Idle', 'Adv.', 'Scan', 'Daily\nSecret', 'TEK', 'RPI', 'AEM']
    xs = [consumptions[IDLE_LABEL], consumptions['adv'], consumptions['scan'], consumptions['generate_tek'], consumptions['derive_tek'], consumptions['derive_rpi'], consumptions['encrypt_aem']]

    fig, ax = plt.subplots()

    ax.set_ylabel('Avg. Consumption [mA]')
    ax.set_xlabel('Functionality')

    bars = ax.bar(ys,xs)
    ax.bar_label(bars, padding=5, fmt='%.2f')

    # Adapt the figure size as needed
    fig.set_size_inches(4.5, 3.2)
    plt.tight_layout()
    plt.savefig("../out/current_per_functionality.pdf", format="pdf", bbox_inches='tight')
    plt.close()

def export_tek_check():
    tek_check_duration = 0.020

    xs = [1000, 10000, 1000000]
    ys = [x*tek_check_duration for x in xs]

    xs = [str(x) for x in xs]

    fig, ax = plt.subplots()

    ax.set_ylabel('Extrapolated Time [s]')
    ax.set_xlabel('Number of TEKs')

    bars = ax.bar(xs,ys)
    ax.bar_label(bars, padding=5, fmt='%d')

    # Adapt the figure size as needed
    fig.set_size_inches(4.5, 3.2)
    plt.tight_layout()
    plt.savefig("../out/tek_check.pdf", format="pdf", bbox_inches='tight')
    plt.close()


export_adv_consumption()
export_usage_seconds_per_day()
export_consumption_per_day()
export_current_per_functionality()
export_tek_check()