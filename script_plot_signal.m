%% Plot signal ARP assignemnt
clc; close all;
%% PUT HERE THE 2 FREQUENCIES USED DURING THE EXPERIMENT
f1 = "10";
f2 = "50";
%% Plot the results
str = 'results/signal_';
file1 = strcat(str,f1,'.txt');
file2 = strcat(str,f2,'.txt');
signal1 = load(file1);
signal2 = load(file2);
sig1_length = length(signal1);
sig2_length = length(signal2);
if (sig1_length < sig2_length) 
    len = length(signal1);
else
    len = length(signal2);
end
t = [1:len];

plot(t,signal1(1:len),'k','LineWidth',2);
hold on
plot(t,signal2(1:len),'r','LineWidth',2);
legend(strcat(f1,'Hz'),strcat(f2,'Hz'));
xlabel('Time (s)')
ylabel('Amplitdue')