#include "rotating_log.h"
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace jishsfirstpersonmod;
std::string read(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};
}
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() {
    try {
        const auto root=std::filesystem::current_path()/"rotation-test-data";
        std::filesystem::create_directories(root);
        for(const auto name:{"events.log","trace.csv"}) {
            const auto file=root/name;
            for(unsigned i=0;i<=3;++i) std::filesystem::remove(i ? log_archive(file,i):file);
        }
        const auto text=root/"events.log";
        for(unsigned i=0;i<8;++i) {
            const auto message="event="+std::to_string(i)+"\n";
            check(append_rotating_log(text,message,8),"text append failed");
        }
        check(read(text)=="event=7\n" && read(log_archive(text,1))=="event=6\n" &&
              read(log_archive(text,3))=="event=4\n","latest text history not retained");
        check(!std::filesystem::exists(log_archive(text,4)),"retention exceeded three archives");
        const auto csv=root/"trace.csv";
        for(unsigned i=0;i<6;++i)
            check(append_rotating_log(csv,"1,2\n",10,3,"a,b\n"),"CSV append failed");
        for(unsigned i=0;i<=3;++i) {
            const auto file=i ? log_archive(csv,i):csv;
            check(read(file)=="a,b\n1,2\n","rotated CSV header/row invalid");
            check(std::filesystem::file_size(file)<=10,"rotated file exceeds budget");
        }
        // An archive locked/blocked by another tool must not permanently stop capture.
        std::filesystem::remove(log_archive(text,3));
        std::filesystem::create_directory(log_archive(text,3));
        { std::ofstream marker(log_archive(text,3)/"held");marker<<"held"; }
        check(append_rotating_log(text,"event=8\n",8),"blocked rotation stopped logging");
        check(read(text).ends_with("event=8\n"),"fallback append lost new record");
        std::filesystem::remove(log_archive(text,3)/"held");std::filesystem::remove(log_archive(text,3));
        check(append_rotating_log(text,"event=9\n",8),"rotation did not recover");
        check(read(text)=="event=9\n","recovered log did not rotate");
        std::cout<<"Retention, latest history, CSV headers and blocked-rotation recovery passed\n";
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
