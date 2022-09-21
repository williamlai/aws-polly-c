# AWS Polly C

AWS Polly C is a lightweight library of AWS Polly service for FreeRTOS/Embedded Linux. This library doesn't provide full features of AWS Polly but only supports some key features of it.

## What is AWS Polly?

Amazon Polly is a cloud service that converts text into spoken audio. Its Text-to-Speech (TTS) service uses deep learning to synthesize natural sounding human speech.

## How to use this library

### Download the source code

First, run the following command to download the source code.

```
git clone https://github.com/williamlai/aws-polly-c.git --recursive
```

### Configure and build the project

Run the following command to configure the CMake project.

```
mkdir build
cd build
cmake ..
```

Build the project with the following command.

```
cmake --build .
```

### Configure and run the sample code

When the build is complete, there is a "polly_mp3_download" sample in the `bin` folder. This sample accepts a few sentences as input, uses AWS Polly TTS to synthesize a speech, and then stores it into an MP3 file.

Before running this sample, you need to configure your AWS credentials and regions in the environment variable. You can configure them with the following command.

```
export AWS_ACCESS_KEY_ID=xxxxxxxxxxxxxxxxxxxx
export AWS_SECRET_ACCESS_KEY=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export AWS_DEFAULT_REGION=us-east-1
```

Here is the usage of the sample.

```
polly_mp3_download <OutputFilename.mp3> "<Text>"
```

For example, you can run this sample with the following command

```
./bin/polly_mp3_download temp.mp3 "It's a lovely day today. So whatever you've got to do, you've got a lovely day to do."
```

There will be a `temp.mp3` file in the same folder you execute the command. You can use a music player to verify the results.
