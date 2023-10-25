#include <iostream>
#include <mutex>
#include <fstream>
#include <queue>
#include <Windows.h>
#include "pthread.h"
#pragma comment(lib, "pthreadVCE2.lib")

#define DEBUG_MODE
constexpr auto INPUT_STRING = "\\input.txt";
constexpr auto OUTPUT_STRING = "\\output.txt";

void* read_file(void* file_path_ptr);
void* gcd(void* args);
void* write_file(void* file_path_ptr);

struct Task {
	int a;
	int b;
};

class QueueWithBoolBase {
public:
	bool is_complete = false;
};

template <typename T>
class QueueWithBool : public QueueWithBoolBase {
public:
	std::queue<T> queue;
};

QueueWithBool<Task> read_queue;
QueueWithBool<int> processed_queue;

std::mutex read_mutex;
std::mutex write_mutex;

int main() {
	setlocale(LC_ALL, "rus");
	pthread_t reading_thread;
	pthread_t gcd_thread;
	pthread_t writing_thread;
	
	std::string input_file = INPUT_STRING;
	std::string output_file = OUTPUT_STRING;

	pthread_create(&reading_thread, NULL, read_file, &input_file);
	pthread_create(&gcd_thread, NULL, gcd, NULL);
	pthread_create(&writing_thread, NULL, write_file, &output_file);

	pthread_join(reading_thread, NULL);
	pthread_join(gcd_thread, NULL);
	pthread_join(writing_thread, NULL);
#ifdef DEBUG_MODE
	getchar();
#endif
	return 0;
}

void* read_file(void *file_path_ptr) {
	int a, b;
	std::string file_path = *static_cast<std::string*>(file_path_ptr);
	// Get the path to the executable
	std::string exePath = __FILE__; // __FILE__ is the path of the current source file
	size_t found = exePath.find_last_of("/\\");
	std::string folderPath = exePath.substr(0, found);
	std::string inputFilePath = folderPath + file_path;

	std::ifstream infile(inputFilePath);
	if (infile.is_open()) {
		while (infile >> a >> b)
		{
			Task task = { a, b };
#ifdef DEBUG_MODE
			printf("reading: %d, %d\n", task.a, task.b);
#endif
			read_mutex.lock();
			read_queue.queue.push(task);
			read_mutex.unlock();
		}

		infile.close(); // Close the file when done
	}
	else {
		std::cerr << "Failed to open the input file." << std::endl;
	}
#ifdef DEBUG_MODE
	printf("reading completed\n");
#endif
	read_queue.is_complete = true;
	infile.close();
	return NULL;
}

void* gcd(void* args)
{
	while (!read_queue.is_complete || !read_queue.queue.empty()) {
		//Sleep(100);
		if (read_queue.queue.empty())
			continue;
		// read task from collection
		read_mutex.lock();
		Task task = read_queue.queue.front();
		read_queue.queue.pop();
		read_mutex.unlock();

		// process task
		int a = task.a, b = task.b, c;
		while (b != 0)
		{
			c = a % b;
			a = b;
			b = c;
		}

		// write result
#ifdef DEBUG_MODE
		printf("gcd: %d\n", a);
#endif
		write_mutex.lock();
		processed_queue.queue.push(a);
		write_mutex.unlock();
	}
	processed_queue.is_complete = true;
#ifdef DEBUG_MODE
	printf("processing completed.\n");
#endif
	return NULL;
}

void* write_file(void* file_path_ptr) {

	std::string file_path = *static_cast<std::string*>(file_path_ptr);
	// Get the path to the executable
	std::string exePath = __FILE__; // __FILE__ is the path of the current source file
	size_t found = exePath.find_last_of("/\\");
	std::string folderPath = exePath.substr(0, found);
	std::string outputFilePath = folderPath + file_path;
	std::ofstream file(outputFilePath);

	if (file.is_open()) {
		while (!processed_queue.is_complete || !processed_queue.queue.empty()) {
			if (processed_queue.queue.empty()) {
				//Sleep(100); // for demonstration purposes
				continue;
			}
			// get processed result
			write_mutex.lock();
			int result = processed_queue.queue.front();
			processed_queue.queue.pop();
			write_mutex.unlock();
#ifdef DEBUG_MODE
			printf("writing: %d\n", result);
#endif
			//write processed result
			file << result << std::endl;
		}
		file.close();
	}
	else {
		std::cerr << "Failed to open the output file." << std::endl;
	}
#ifdef DEBUG_MODE
	std::cout << "Writing completed.\n";
#endif
	return NULL;
}
