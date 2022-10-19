#! /usr/bin/env bash

# 4.1.1
python plot_cycles.py ../outputs/4.1.1/*001.out/
python plot_cycles.py ../outputs/4.1.1/*010.out/
python plot_cycles.py ../outputs/4.1.1/*100.out/

# 4.1.3
python plot_energy.py ../outputs/4.1.1/*001.out/
python plot_energy.py ../outputs/4.1.1/*010.out/
python plot_energy.py ../outputs/4.1.1/*100.out/

# 4.1.4
python plot_time_real_system.py ../outputs/4.1.4/*001.out/
python plot_time_real_system.py ../outputs/4.1.4/*010.out/
python plot_time_real_system.py ../outputs/4.1.4/*100.out/

# 4.2.1
python plot_time-threads.py ../outputs/4.2.1/*
