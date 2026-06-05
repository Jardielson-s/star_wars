## Description
<p> Game base on star wars</>

## Tools
* glad
* soLoud
* lassimp

## Install
```
sudo apt update
sudo apt install build-essential git libasound2-dev pulseaudio libpulse-dev

```


## Run
```
g++ main.cpp ../libs/soloud/src/core/*.cpp ../libs/soloud/src/backend/miniaudio/*.cpp ../libs/soloud/src/audiosource/wav/*.cpp ../libs/soloud/src/audiosource/wav/*.c glad.c -I./include -I. -I../libs/soloud/include -I../libs/soloud/src/backend/miniaudio -DWITH_MINIAUDIO -Wno-write-strings -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lassimp -lasound -o game


```

