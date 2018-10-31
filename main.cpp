#include <iostream>
#include <string>
#include <list>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

void print_res(list<string>& strings) {
    for(auto it=strings.cbegin(); it!=strings.cend(); ++it) {
        cout << *it << "|\n";
    }
}

list<string> split(string s, string delimiter) {

    list<string> strings;

    size_t pos = 0;
    std::string token;

    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        strings.push_back(token);
        s.erase(0, pos + delimiter.length());
    }

    strings.push_back(s);

    return strings;
}

list<list<string>> coms(string& str) {

    list<list<string>> result;
    list<string> strings = split(str, " | ");

    for(auto it=strings.begin(); it!=strings.end(); ++it) {
        auto s = split(*it, " ");
        result.push_back(s);
    }

    return result;
}

char** convert(list<string>& strings) {

    char** coms = new char*[strings.size() + 1];
    size_t i = 0;

    for(auto it=strings.begin(); it!=strings.end(); ++it) {
        coms[i] = const_cast<char*>(it->c_str());
        ++i;
    }

    coms[i] = NULL;

    return coms;
}


void multi(list<list<string>> coms) {

    int num_commands = coms.size();
    int num_pipes = num_commands - 1;

    /* parent creates all needed pipes at the start */
    int pipefds[2*num_pipes];

    for(int i = 0; i < num_pipes; i++)
       if( pipe(pipefds + i*2) < 0 )
            cout << "Error:" << strerror(errno) << "\n";

    int commandc = 0;
    while( !coms.empty() ){

        char** argv = convert(coms.front());
        coms.pop_front();

        pid_t pid = fork();

        if( pid == 0 ){

            /* child gets input from the previous command,
                if it's not the first command */
            if(commandc != 0) {
                if( dup2(pipefds[(commandc-1)*2], 0) < 0 )
                    cout << "Error1:" << strerror(errno) << "\n";
            }

            /* child outputs to next command, if it's not
                the last command */
            if(commandc != num_commands-1) {
                if( dup2(pipefds[commandc*2+1], 1) < 0 )
                    cout << "Error2:" << strerror(errno) << "\n";
            } else {
//                int fd = open("/home/box/result.out", O_RDWR | O_CREAT | O_TRUNC, 0666);
                int fd = open("./result.out", O_RDWR | O_CREAT | O_TRUNC, 0666);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            for(int i = 0; i < 2 * num_pipes; i++)
                close( pipefds[i] );

            if (execvp(argv[0], argv) < 0)
                cout << "Error:" << strerror(errno) << "\n";


        } else if( pid < 0 ){
            cout << "Error:" << strerror(errno) << "\n";
        }

        ++commandc;
    }    

    for(int i = 0; i < 2 * num_pipes; i++)
        close( pipefds[i] );
}

void to_stdin(string str) {
    stringstream msg;
    msg << str;
    auto s = msg.str();
    write(STDIN_FILENO, s.c_str(), s.size());
    close(STDIN_FILENO);
}


int main()
{
    std::string str;
    std::getline(std::cin, str);

    cout << "\nStr: " << str;
//    string str = "who | sort | uniq -c | sort -nk1";
//    string str = "who | wc -l";
//    string str = "ls -a -f";

    auto com = coms(str);
    multi(com);

    return 0;
}

