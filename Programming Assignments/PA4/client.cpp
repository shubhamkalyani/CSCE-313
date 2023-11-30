#include <fstream>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "BoundedBuffer.h"
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "TCPRequestChannel.h"

using namespace std;

struct Response {
    int person;
    double ecg;
};

void patient_thread_function(int patientID, int numRequests, BoundedBuffer* requestBuffer) {
    datamsg dataRequest(patientID, 0.00, 1);
    for(int i = 0; i<numRequests; i++) {
        requestBuffer->push((char*) &dataRequest, sizeof(datamsg));
        dataRequest.seconds += 0.004;
    }
}

void file_thread_function(string file_name, BoundedBuffer* requestBuffer, TCPRequestChannel* channel, int buffer_size) {
    string received_file_name = "received/" + file_name;
    char buffer[1024];
    filemsg file_message(0, 0);
    memcpy(buffer, &file_message, sizeof(file_message));
    strcpy(buffer + sizeof(file_message), file_name.c_str());
    channel->cwrite(buffer, sizeof(file_message) + file_name.size() + 1);
    __int64_t file_length;
    channel->cread(&file_length, sizeof(file_length));

    FILE* file_pointer = fopen(received_file_name.c_str(), "w");
    fseek(file_pointer, file_length, SEEK_SET);
    fclose(file_pointer);

    filemsg* file_msg = (filemsg*) buffer;
    __int64_t remaining_length = file_length;

    while (remaining_length > 0) {
        file_msg->length = min(remaining_length, (__int64_t) buffer_size);
        requestBuffer->push(buffer, sizeof(filemsg) + file_name.size() + 1);
        file_msg->offset += file_msg->length;
        remaining_length -= file_msg->length;
    }
}

void worker_thread_function(BoundedBuffer* reqBuffer, TCPRequestChannel* channel, BoundedBuffer* respBuffer, int bufferSize) {
    const int MAX_BUFFER_SIZE = 1024;
    char request[MAX_BUFFER_SIZE];
    char* receivedBuffer = new char[bufferSize];

    while(true) {
        reqBuffer->pop(request, sizeof(request));
        auto messageType = reinterpret_cast<MESSAGE_TYPE*>(request);

        if(*messageType == QUIT_MSG) {
            channel->cwrite(messageType, sizeof(MESSAGE_TYPE));
            delete channel;
            break;
        }
        if(*messageType == DATA_MSG) {
            auto dataMessage = reinterpret_cast<datamsg*>(request);
            channel->cwrite(dataMessage, sizeof(datamsg));

            double ecgValue;
            channel->cread(&ecgValue, sizeof(double));

            Response response{dataMessage->person, ecgValue};
            respBuffer->push(reinterpret_cast<char*>(&response), sizeof(response));
        } else if (*messageType == FILE_MSG) {
            auto fileMessage = reinterpret_cast<filemsg*>(request);
            std::string filename = std::string(reinterpret_cast<char*>(fileMessage + 1));

            int dataSize = sizeof(filemsg) + filename.size() + 1;
            channel->cwrite(request, dataSize);
            channel->cread(receivedBuffer, bufferSize);

            std::string receivedFilename = "received/" + filename;

            FILE* filePointer = fopen(receivedFilename.c_str(), "r+");
            fseek(filePointer, fileMessage->offset, SEEK_SET);
            fwrite(receivedBuffer, 1, fileMessage->length, filePointer);
            fclose(filePointer);
        }
    }
    delete[] receivedBuffer;
}

void histogram_thread_function (BoundedBuffer* responseBuffer, HistogramCollection* hc){
    char buf [1024];
    while(true) {
        responseBuffer->pop(buf,1024);
        Response* r = (Response*) buf;
        if(r->person == -1) {
            break;
        }
        hc->update(r->person, r->ecg);
    }
}

int main(int argc, char *argv[]) {
    int opt;
    int n = 1000;
    int p = 10;
    int w = 100;
    int h = 20;
    int b = 20;
    int m = MAX_MESSAGE;
    srand(time_t(NULL));
    string f = "";
    string ip_address;
    string port_no;
    
    while((opt = getopt(argc, argv, "n:p:w:h:b:m:f:a:r:")) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'p':
                p = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'm':
                m = atoi(optarg);
                break;
            case 'f':
                f = optarg;
                break;
            case 'h':
                h = atoi(optarg);
                break;
            case 'a':
                ip_address = optarg;
                break;
            case 'r':
                port_no = optarg;
                break;
        }
    }
    
    TCPRequestChannel* chan = new TCPRequestChannel(ip_address, port_no);
    BoundedBuffer requestBuffer(b);
    BoundedBuffer responseBuffer(b);
    HistogramCollection hc;

    vector<thread> patients(p);
    vector<thread> workers(w);
    vector<thread> hists(h);
    vector<TCPRequestChannel*> wchans(w);

    for (int i = 0; i < w; i++) {
        wchans[i] = new TCPRequestChannel(ip_address, port_no);
    }

    for (int i = 0; i < p; i++) {
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }

    struct timeval start, end;
    gettimeofday(&start, 0);

    if (f == "") {
        for (int i = 0; i < p; i++) {
            patients[i] = thread(patient_thread_function, i + 1, n, &requestBuffer);
        }

        for (int i = 0; i < w; i++) {
            workers[i] = thread(worker_thread_function, &requestBuffer, wchans[i], &responseBuffer, m);
        }

        for (int i = 0; i < h; i++) {
            hists[i] = thread(histogram_thread_function, &responseBuffer, &hc);
        }
    } else {
        thread filethread(file_thread_function, f, &requestBuffer, chan, m);

        for (int i = 0; i < w; i++)
            workers[i] = thread(worker_thread_function, &requestBuffer, wchans[i], &responseBuffer, m);

        filethread.join();
    }
    
    for (int i = 0; i < p; i++) {
        patients[i].join();
    }

    for (int i = 0; i < w; i++) {
        MESSAGE_TYPE q = QUIT_MSG;
        requestBuffer.push((char*) &q, sizeof(MESSAGE_TYPE));
    }

    for (int i = 0; i < w; i++) {
        workers[i].join();
    }
    
    Response r {-1, 0}; 

    for (int i = 0; i < h; i++)
        responseBuffer.push((char*) &r, sizeof(r));

    for (int i = 0; i < h; i++)
        hists[i].join();
    
    gettimeofday(&end, 0);

	hc.print();
    int secs = ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int) 1e6);
    int usecs = (int) ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    wait(0);
    delete chan;
    // wait(nullptr);
    wait(nullptr);
}
