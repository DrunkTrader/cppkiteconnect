const char* compileConsumerA();
const char* compileConsumerB();

int main() {
    return compileConsumerA() == nullptr || compileConsumerB() == nullptr;
}
