#include <iostream>
#include <string>
using namespace std;

int main() {
  // Flush after every std::cout / std:cerr
  
  cout << unitbuf;
  cerr << unitbuf;
  
  // TODO: Uncomment the code below to pass the first stage
 
  cout << "$ ";
  string command;
  getline(cin,command);

  if(command=="exit"){
    return 0;
  }

  string prefix="echo ";

if(command.rfind(prefix,0)==0){
  cout<<command.substr(prefix.size())<< '\n';
  main();
}
 string prefix2="type ";

if(command.rfind(prefix2,0)==0){
  if(command.substr(prefix.size()) =="exit"||command.substr(prefix.size())=="echo" || command.substr(prefix.size())=="type"){
    cout<<command.substr(prefix.size())<<" is a shell builtin"<< '\n';
    main();
  }
}

  cout << command <<": command not found"<< endl;


  main();
}
