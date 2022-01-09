fs = 48e3;
N = 5;
x=rand(N*fs,1)*2-1;
X=fft(x);
%loglog(abs(real(X)));
%magplot(X)
fLow=1000;
fHigh=4000;
X(1:fLow*N)=10.^(-300/20);
X(fHigh*N:end)=10.^(-300/20);

bandIndices = fLow*N-1:fHigh*N+2; % +2 for compensation
X(bandIndices)=exp(1i * (x(bandIndices))); % use random phase (reuse input signal)

%loglog(abs(real(X)))
%magplot(X);
y=real(ifft(X));
%loglog(real(fft(y)))
%y=y./max(abs(y));
%magplot(fft(y));
%plot(y)
audiowrite('BandLimitedNoise_1k_4k.wav', y, fs);

%angle(z) = imag(log(z)) = atan2(imag(z),real(z)).