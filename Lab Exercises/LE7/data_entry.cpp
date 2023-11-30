#include <vector> // vector, push_back, at
#include <string> // string
#include <cstring>
#include <iostream> // cin, getline
#include <fstream> // ofstream
#include <unistd.h> // getopt, exit, EXIT_FAILURE
#include <assert.h> // assert
#include <thread> // thread, join
#include <sstream> // stringstream

#include "BoundedBuffer.h" // BoundedBuffer class

#define MAX_MSG_LEN 256

using namespace std;

/************** Helper Function Declarations **************/

void parse_column_names(vector<string>& _colnames, const string& _opt_input);
void write_to_file(const string& _filename, const string& _text, bool _first_input=false);

/************** Thread Function Definitions **************/

void ui_thread_function(BoundedBuffer* bb) {
    do {
        string input;
        cout << "enter data> ";
        getline(cin, input);

        if (input == "Exit" || input == "exit") {
            bb->push(nullptr, 0);
            break;
        }

        const int len = input.length();
        char* newdata = new char[len + 1];
        strcpy(newdata, input.c_str());

        bb->push(newdata, input.size());
        delete[] newdata;

    } while (true);
}

string formatMessage(const string& message, int numCols) {
    static int colCounter = 0;
    stringstream formattedStream;

    colCounter++;
    formattedStream << message;

    if (colCounter % numCols == 0) {
        formattedStream << '\n';
    } else {
        formattedStream << ", ";
    }

    return formattedStream.str();
}

void data_thread_function(BoundedBuffer* bb, const string& filename, const vector<string>& colnames) {
    const int numCols = colnames.size();
    
    while (true) {
        char messageBuffer[MAX_MSG_LEN];
        int bytesRead = bb->pop(messageBuffer, MAX_MSG_LEN);

        if (bytesRead == 0) {
            break;
        }

        string message(messageBuffer, bytesRead);
        string formattedMessage = formatMessage(message, numCols);

        write_to_file(filename, formattedMessage, false);
    }
}

/************** Main Function **************/

int main(int argc, char* argv[]) {

    // variables to be used throughout the program
    vector<string> colnames; // column names
    string fname; // filename
    BoundedBuffer* bb = new BoundedBuffer(3); // BoundedBuffer with cap of 3

    // read flags from input
    int opt;
    while ((opt = getopt(argc, argv, "c:f:")) != -1) {
        switch (opt) {
            case 'c': // parse col names into vector "colnames"
                parse_column_names(colnames, optarg);
                break;
            case 'f':
                fname = optarg;
                break;
            default: // invalid input, based on https://linux.die.net/man/3/getopt
                fprintf(stderr, "Usage: %s [-c colnames] [-f filename]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    write_to_file(fname, "", true);
    for (size_t i = 0; i < colnames.size(); i++){
        if (i == colnames.size() - 1){
            string header = colnames[i] + "\n";
            write_to_file(fname, header.c_str(), false);
        } else {
            string header = colnames[i] + ", ";
            write_to_file(fname, header.c_str(), false);
        }
    }

    thread ui = thread(ui_thread_function,bb);
    thread data = thread(data_thread_function,bb,fname,colnames);

    ui.join();

    data.join();

    delete bb;

    return 0;
}

/************** Helper Function Definitions **************/

// function to parse column names into vector
// input: _colnames (vector of column name strings), _opt_input(input from optarg for -c)
void parse_column_names(vector<string>& _colnames, const string& _opt_input) {
    stringstream sstream(_opt_input);
    string tmp;
    while (sstream >> tmp) {
        _colnames.push_back(tmp);
    }
}

// function to append "text" to end of file
// input: filename (name of file), text (text to add to file), first_input (whether or not this is the first input of the file)
void write_to_file(const string& _filename, const string& _text, bool _first_input) {
    // based on https://stackoverflow.com/questions/26084885/appending-to-a-file-with-ofstream
    // open file to either append or clear file
    ofstream ofile;
    if (_first_input)
        ofile.open(_filename);
    else
        ofile.open(_filename, ofstream::app);
    if (!ofile.is_open()) {
        perror("ofstream open");
        exit(-1);
    }

    // sleep for a random period up to 5 seconds
    usleep(rand() % 5000);

    // add data to csv
    ofile << _text;

    // close file
    ofile.close();
}