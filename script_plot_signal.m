%% Plot signal ARP assignemnt
clc; close all;
load('results/signal_10.txt');
load('results/signal_2.txt');
sig1_length = length(signal_10);
sig2_length = length(signal_2);
if (sig1_length < sig2_length) 
    len = length(signal_10);
else
    len = length(signal_2);
end
t = [1:len];

plot(t,signal_10(1:len),'k','LineWidth',2);
hold on
plot(t,signal_2(1:len),'r','LineWidth',2);

