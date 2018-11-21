#include <iostream>
#include <string>
#include <sstream>

int main()
{
    std::stringstream ss;
    int n=1234;

    ss << n;

    std::string str = ss.str();
    std::cout << str << std::endl;


//    char msg[] = "aaa bbb ccc";
//    std::stringstream ss(msg);
//    std::string p1, p2, p3;
//
//    ss >> p1 >> p2 >> p3;
//
//    std::cout << p1 << std::endl;
    return 0;
}
