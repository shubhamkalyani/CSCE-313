#include "common.h"
#include "FIFORequestChannel.h"
using namespace std;

int main (int argc, char *argv[]) {
    pid_t pid = fork();
    if (pid == 0) {
        char* args[] = {(char*) "./server", nullptr};
        execvp(args[0], args);
    } 
    int opt;
    int p = 1;
    double t = 0;
    int e = 1;
    int c = 0;
    string filename = "";
  
    while ((opt = getopt(argc, argv, "p:t:e:f:c")) != -1) {
        switch (opt) {
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                e = atoi (optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'c':
                c = 1;
                break;
        }
    }

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

    char buf[MAX_MESSAGE]; // 256
    char buff[MAX_MESSAGE];

    if (t == 0.0 && e == 1) {
        ofstream x1("received/x1.csv");
        for (int i = 0; i < 1000; i++) {
            // x1 << 
            datamsg x(p, t, 1);
            memcpy(buf, &x, sizeof(datamsg));
            chan.cwrite(buf, sizeof(datamsg)); // question
            double reply;
            chan.cread(&reply, sizeof(double)); //answer

            datamsg y(p, t, 2);
            memcpy(buff, &y, sizeof(datamsg));
            chan.cwrite(buff, sizeof(datamsg)); // question
            double reply2;
            chan.cread(&reply2, sizeof(double)); //answer

            x1 << t << "," << reply << "," << reply2 << "\n";

            t += 0.004;
        }
        x1.close();
    } else {
        datamsg x(p, t, e);
        memcpy(buf, &x, sizeof(datamsg));
        chan.cwrite(buf, sizeof(datamsg)); // question
        double reply;
        chan.cread(&reply, sizeof(double)); //answer
        cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
    }

    if (!filename.empty()) {
        filemsg fm(0, 0);
        string fname = filename;
        int len = sizeof(filemsg) + (fname.size() + 1);
        char *buf2 = new char[len];
        memcpy(buf2, &fm, sizeof(filemsg));
        strcpy(buf2 + sizeof(filemsg), fname.c_str());
        chan.cwrite(buf2, len);
        delete[] buf2;

        __int64_t fileLength;
        chan.cread(&fileLength, sizeof(__int64_t));

        const int buffCap = MAX_MESSAGE - sizeof(filemsg);
        const __int64_t chunkSize = buffCap;
        int numChunks = (fileLength + chunkSize - 1) / chunkSize;

        for (int i = 0; i < numChunks; i++) {
            __int64_t offset = i * chunkSize;
            int bytesToTransfer = fileLength - offset;
            int chunkLength = (bytesToTransfer < chunkSize) ? bytesToTransfer : chunkSize;

            filemsg chunkMessage(offset, chunkLength);

            int chunkMessageSize = sizeof(filemsg);
            char *chunkBuffer = new char[chunkMessageSize + fname.size() + 1];
            memcpy(chunkBuffer, &chunkMessage, chunkMessageSize);
            strcpy(chunkBuffer + chunkMessageSize, fname.c_str());

            chan.cwrite(chunkBuffer, chunkMessageSize + fname.size() + 1);

            char *chunkData = new char[chunkLength];
            chan.cread(chunkData, chunkLength);

            ofstream outFile("received/" + fname, ios::app | ios::binary);
            outFile.write(chunkData, chunkLength);
            outFile.close();

            delete[] chunkBuffer;
        }

        string received = "received/" + fname;
        string original = "BIMDC/" + fname;
        string diffCommand = "diff " + received + " " + original;
        int diffCheck = system(diffCommand.c_str());

        if (diffCheck == 0) {
            cout << "File received successfully and matches the original." << endl;
        } else {
            cout << "File received but does not match the original." << endl;
        }
    }

    if (c != -1){
        MESSAGE_TYPE newChannelMsg = NEWCHANNEL_MSG;
        chan.cwrite(&newChannelMsg, sizeof(MESSAGE_TYPE));

        char newChannelName[256];
        chan.cread(newChannelName, sizeof(newChannelName));

        FIFORequestChannel newChan(newChannelName, FIFORequestChannel::CLIENT_SIDE);

        datamsg x(p, t, e);
        memcpy(buf, &x, sizeof(datamsg));
        newChan.cwrite(buf, sizeof(datamsg));
        double reply;
        newChan.cread(&reply, sizeof(double));
        cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
    }
    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
}
