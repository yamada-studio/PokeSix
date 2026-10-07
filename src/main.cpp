#include "app/application.h"

// 실행 파일은 부트스트랩(app 레이어)을 만들고 돌리기만 한다. 객체를 만들고 잇는 일은 전부
// Application(src/app/application.cpp)에 있다 — architecture.md §2 "app".
int main(int argc, char *argv[])
{
    com::yamada::studio::Application application(argc, argv);
    return application.run();
}
