#include <memory>
#include <vector>

struct Widget;

std::vector<std::shared_ptr<Widget>> pw;


struct Widget : public std::enable_shared_from_this<Widget> {

    
};

int main()
{
    auto pw = new Widget;

    std::shared_ptr<Widget> spw1(pw);
    {
        std::shared_ptr<Widget> spw2(pw); // double deletion on same pw
    }

    return 0;
}