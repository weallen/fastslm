%%
[time, X, Y] = textread('spiral_xy_125khz_12-10-13.atf');
[time, x] = textread('spiral_x_250khz_12-10-13.atf');
[time, y] = textread('spiral_y_250khz_12-10-13.atf');

%% TODO compare with lores version

%%
centeringIdxStartX = 401648+5;
centeringIdxStartY = 374108-1;
centeringStartX = centeringIdxStartX;
centeringStopX = centeringIdxStartX+360;
centeringStartY = centeringIdxStartY;
centeringStopY = centeringIdxStartY+360;

centeringIdxStart = 360956+116; 
centeringIdxStop = centeringIdxStart + 179;

plot(x(centeringStartX:2:centeringStopX),'r');
hold on;
plot(y(centeringStartY:2:centeringStopY),'r');
hold on;
plot(X(centeringIdxStart:centeringIdxStop),'b');
hold on;
plot(Y(centeringIdxStart:centeringIdxStop),'b');
recenteringStartX = 0;
recenteringStopX = 0;

%%
%waveformStartX = centeringStopX+5;
%waveformStartY = centeringStopY;
waveformStartX = centeringStopX;
waveformStartY = centeringStopY;
N = 5;
%offset = repmat(1:N, ;
t = 641;
data = x(waveformStartX+t:(waveformStartX+2*t));
plot(x(waveformStartX:(waveformStartX+N*t)));
%data = x(220:(220+10*t));

hold on;
plot(repmat(data,N,1), 'r');


%%
t = 642;
dataRepsX = zeros(t+1, N);
dataRepsY = zeros(t+1, N);

for i=1:N
    idxX = (waveformStartX + (i-1)*t ):(waveformStartX + i*t);
    idxY = (waveformStartY + (i-1)*t ):(waveformStartY + i*t);
    numel(idx)
    dataRepsX(:,i) = x(idxX);    
    dataRepsY(:,i) = y(idxY);
end
dataRepsX(1,:)
dataRepsX(end,:)
plot(dataRepsX,'r');
hold on;
plot(dataRepsY, 'b');

%%
meanSignalX = mean(dataRepsX,2);
meanSignalY = mean(dataRepsY,2);

plot(meanSignalX,'-');
hold on;
plot(meanSignalY,'-r');




%%
%plot(meanSignalX,'r-');
%hold on;
xq = linspace(0, 500, 10000);
vq = interp1(1:numel(meanSignalX), meanSignalX, xq);
plot(vq);

%% make spline fit and resample
xFit = createFit(meanSignalX);
yFit = createFit(meanSignalY);

xFilt = xFit(linspace(0, 500, 5000));
yFilt = yFit(linspace(0, 500, 5000));

%% save data
fX = fopen('x_galvo.txt','w');
fY = fopen('y_galvo.txt','w');
for i=1:numel(xFilt)
   fprintf(fX, '%10f\n', xFilt(i)); 
   fprintf(fY, '%10f\n', yFilt(i));    
end
fclose(fY);
fclose(fX);    

%%
polar(cart2pol(meanSignalY,meanSignalX));

%% plot archimedean spiral
b = 11;
r = linspace(0, 2*pi,10000); 
theta = b*linspace(0, 2*pi, 10000); 
subplot(1,2,1);
polar(theta,r);
subplot(1,2,2);
plot(0.01*pol2cart(theta,r));