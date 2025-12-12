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

  cout << command <<": command not found"<< endl;


  main();
}
