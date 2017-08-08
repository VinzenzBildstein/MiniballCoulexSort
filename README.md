# MiniballCoulexSort
Analysis code for MINIBALL experiments at ISOLDE. This code is designed to take the raw med data files and convert them to ROOT trees, then subsequently analyse them to produce particle-gated, Doppler corrected spectra. It is broken down in to 3 parts:

### MedToRoot
Based on (read: essentially a copy of) code from Vinzenz Bilstein.
Written originally for transfer expeirments with T-REX, adapted for Coulex with C-REX and now finally for Coulex with SPEDE or with the standard Coulex configuration.

### TreeBuilder
Written by Liam Gaffney, inpsired by initial work by Andres Illan Sison. Daniel Cox implemented SPEDE into this code performed a lot of testing. This takes the converted ROOT file and build an 'mbevts' tree of particle-gamma(-gamma) events that can be used for analysing Coulex events.

### TreeAna
The g_clx part of this is based on codes by Oliver Niedemeier, with contributions by Nigel Warr and Michael Albers. However, it has been completely rewritten by Liam Gaffney to include separate classes for filling histograms and performing Doppler correction. There is still much work to do and the current iteration is being developed by Liam Gaffney (CERN) and Daniel Cox (University of Jyväskylä) for the SPEDE parts.

### Basic process for analysis

To generate initial root file
```
cd MedToRoot
make
./MedToRoot -mf data_file.med - sf setting_file.dat -so
```
data_file_Scaler.root & data_file_Source.root will be created.

Scaler file is needed during the experiment and Source file is needed for analysis.

```
cd TreeBuilder
make
./TreeBuilder -i data_file_Source1.root [data_file_Source2.root ...] -o Output_tree.root -c calibration_file.dat
```
This step takes a number of possible flags:
        [-s                      : singles]
        [-addback                : addback]
        [-crex                   : CREX]
        [-spede                  : SPEDE]
        [-cdpad                  : CD-PAD]
        [-ionch                  : IonChamber]
        [-vl                     : verbose]

Generated file can now be used as input to final analysis code

```
cd ../TreeAna
make
./g_clx -i Output_tree.root -o Final_Output_hists.root
```


