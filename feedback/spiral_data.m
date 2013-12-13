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

plot(x(centeringStartX:centeringStopX),'r');
hold on;
plot(y(centeringStartY:centeringStopY),'b');
%hold on;
%plot(X(centeringIdxStart:centeringIdxStop+1),'b');
%hold on;
%plot(Y(centeringIdxStart:centeringIdxStop+1),'b');

%%
xCenter = X(centeringIdxStart:centeringIdxStop+1);
yCenter = Y(centeringIdxStart:centeringIdxStop+1);

%%
xCenter = x(centeringStartX:centeringStopX);
yCenter = y(centeringStartY:centeringStopY);
%%

xq = linspace(0, numel(xCenter), numel(xCenter)*100);
vqX = interp1(1:numel(xCenter), xCenter, xq);
plot(vqX);

yq = linspace(0, numel(yCenter), numel(yCenter)*100);
vqY = interp1(1:numel(yCenter), yCenter, yq);
plot(vqX(101:end)); hold on; plot(vqY(101:end),'r');
vqX = vqX(101:end);
vqY = vqY(101:end);

fXCenter = fopen('x_center.txt','w');
fYCenter = fopen('y_center.txt','w');
for i=1:numel(xCenter)
   fprintf(fXCenter, '%10f\n', xCenter(i)); 
   fprintf(fYCenter, '%10f\n', yCenter(i));    
end
fclose(fYCenter);
fclose(fXCenter);    

%%
%waveformStartX = centeringStopX+5;
%waveformStartY = centeringStopY;
waveformStartX = centeringStopX;
waveformStartY = centeringStopY;
waveformStart = centeringIdxStop;
N = 5;
%offset = repmat(1:N, ;
t = 641;
data = x(waveformStartX+t:(waveformStartX+2*t));
plot(x(waveformStartX:(waveformStartX+N*t)));
%data = x(220:(220+10*t));

hold on;
plot(repmat(data,N,1), 'r');


%%
N = 10;
t = 643;
t_low = 322;
dataRepsX = zeros(t+1, N);
dataRepsY = zeros(t+1, N);
dataRepsX_low = zeros(t_low+1,N);
dataRepsY_low = zeros(t_low+1,N);

for i=1:N
    idxX = (waveformStartX + (i-1)*t ):(waveformStartX + i*t);
    idxY = (waveformStartY + (i-1)*t ):(waveformStartY + i*t);    
    idx = (waveformStart + (i-1)*t_low):(waveformStart + i*t_low);
    dataRepsX(:,i) = x(idxX);        
    dataRepsY(:,i) = y(idxY);
    dataRepsX_low(:,i) = X(idx);
    dataRepsY_low(:,i) = Y(idx);
end
dataRepsX(1,:)
dataRepsX(end,:)
plot(dataRepsX,'r');
hold on;
plot(dataRepsY, 'b');

%%
meanSignalX = mean(dataRepsX,2);
meanSignalY = mean(dataRepsY,2);
meanSignalX_low = mean(dataRepsX_low,2);
meanSignalY_low = mean(dataRepsY_low,2);

plot(meanSignalX(1:2:numel(meanSignalX)),'-r');
hold on;
plot(meanSignalY(1:2:numel(meanSignalY)),'-b');
%hold on;
%plot(meanSignalX_low,'-b');
%hold on;
%plot(meanSignalY_low,'-b');



%%
%plot(meanSignalX,'r-');
%hold on;
xq = linspace(0, 500, 10000);
vq = interp1(1:numel(meanSignalX), meanSignalX, xq);
plot(vq);

%% make spline fit and resample
xFit = createFit(meanSignalX);
yFit = createFit(meanSignalY);

xFilt = xFit(1:numel(meanSignalX));
yFilt = yFit(1:numel(meanSignalY));

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