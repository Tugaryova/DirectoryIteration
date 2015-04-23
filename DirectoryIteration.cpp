#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace fs = boost::filesystem;

template<typename FunctionType, typename FirstArgumentType, typename SecondArgumentType>
struct Adapter
{
	Adapter(FunctionType f, FirstArgumentType a1, SecondArgumentType& a2) :
		m_function(f),
		m_arg1(a1),
		m_arg2(&a2)
	{}

	void operator()()
	{
		m_function(m_arg1, *m_arg2);
	}

private:
	FunctionType        m_function;
	FirstArgumentType   m_arg1;
	SecondArgumentType* m_arg2;
};

class MultythreadSafeSumma : public boost::noncopyable
{
public:
	MultythreadSafeSumma() :
		m_summa(0)
	{}

	void operator+=(int addend)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		m_summa += addend;
	}

	int GetSumma()
	{
		return m_summa;
	}

private:
	int          m_summa;
	boost::mutex m_mutex;
};

bool StringToInt(const std::string& str, int& number)
{
	bool isValid = true;
	try 
	{
		number = boost::lexical_cast<int>(str);
	}
	catch(boost::bad_lexical_cast& e)
	{
		isValid = false;
	}
	return isValid;
}

void GetNumber(fs::path path, MultythreadSafeSumma& sum)
{
	if (fs::is_regular_file(path))
	{		
		std::string str;
		std::ifstream ifs(path.string().c_str());
		ifs >> str;
		ifs.close();

		std::string fileName = path.filename().string();
		std::stringstream ss;
		ss << fileName << ": ";

		int number = 0;
		if (!StringToInt(str, number))
			ss << "no number\r\n";
		else
			ss << number << "\r\n";
		std::cout << ss.str();
		
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		sum += number;
	}
}

typedef void (*NumberGetter)(fs::path path, MultythreadSafeSumma& sum);

int main(int argc, const char* argv[])
{
	if (argc == 2 && fs::exists(argv[1]))
	{
		MultythreadSafeSumma sum;

		boost::thread_group group;
		
		fs::recursive_directory_iterator end_iter;
		for (fs::recursive_directory_iterator iter(argv[1]); iter != end_iter; ++iter)
		{
			group.add_thread(new boost::thread(Adapter<NumberGetter, fs::path, MultythreadSafeSumma>(GetNumber, iter->path(), sum)));
		}
		group.join_all();
		std::cout << sum.GetSumma() << std::endl;
	}
	else
	{
		std::cout << "Argument is incorrect" << std::endl;
	}
	return 0;
}
