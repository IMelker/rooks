//
// Created by imelker on 21.03.19.
//

#include <iostream>
#include <vector>
#include <tuple>     // std::tie
#include <algorithm> // std::find_if
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

/**
* @brief Тестовое задание Voximplant
* На шахматной доске находятся в произвольной позиции N ладей (4-6)
* Они все одновременно начинают ходить на случайные позиции
* (при этом перемещаться они, естественно, могут только горизонтально либо вертикально).
* Между каждыми ходами каждая фигура делает паузу 200-300 миллисекунд.
* Если на пути фигуры оказывается другая, она ждет, пока путь освободится.
* Если в течение 5 секунд проход не освободился, выбирается другая позиция аналогичным случайным образом.
* Всё заканчивается, когда все фигуры сделают по 50 ходов
*/

constexpr size_t rook_count_min = 4;
constexpr size_t rook_count_max = 6;
constexpr int step_max_count = 50;
constexpr int step_delay_min = 200;
constexpr int step_delay_max = 300;
constexpr int collision_delay_sec = 5;
constexpr int field_size = 8;

std::mutex m;
std::condition_variable cv_move;
std::atomic_bool start(false);

std::vector<class Rook> rooks;

bool onTheMoveWay(const Rook& rook, int new_x, int new_y);
bool posIsTaken(int x, int y);

/**
* @brief  Helper for thread safe cout
*/
template <typename Arg, typename... Args>
void blockingLog(std::ostream &stream, Arg &&arg, Args &&... args) {
  std::unique_lock<std::mutex> ul(m);
  stream << std::forward<Arg>(arg);
  using expander = int[];
  (void)expander{0, (void(stream << std::forward<Args>(args)), 0)...};
  stream << std::endl;
}
/**@{*/

/**
* @brief  Helper get random integer
*/
template <int min = 0, int max = field_size>
int getRandomInt() {
  static std::random_device rd;
  static std::mt19937 mt(rd());
  static std::uniform_int_distribution<int> dist(min, max);
  return dist(mt);
}
/**@{*/

/**
* @brief  Rook class provides run thread and current position
*/
class Rook {
  friend bool onTheMoveWay(const Rook& rook, int new_x, int new_y);
  friend bool posIsTaken(int x, int y);
 public:
  Rook() : num(st_num++), x(-1), y(-1), runnable(&Rook::run, this) {
    std::tie(x,y) = generateDefaultPos();
    std::cout << "rook[" << num << "]: emplaced (" << x << "," << y << ")" << std::endl;
  }

  ~Rook() {
    if(runnable.joinable())
      runnable.join();
  }

 private:
  /**
  * @brief Main method. Wait notify on cv and then starts step loop
  */
  void run() {
    {
      std::unique_lock<std::mutex> lk(m);
      cv_move.wait(lk, [] () ->bool { return start;});
    }

    time_t timestamp = 0;
    int new_x = x, new_y = y;

    for (int step = 0; step < step_max_count; ++step) {
      std::this_thread::sleep_for(std::chrono::milliseconds(getRandomInt<step_delay_min,step_delay_max>()));

      bool is_blocked = isBlocked(timestamp);
      if (!is_blocked) {
        std::tie(new_x, new_y) = generateNextStep();
      }

      if(onTheMoveWay(*this, new_x, new_y)) {
        if (!is_blocked) {
          timestamp = std::time(nullptr);
        }
        --step;
      } else {
        if (is_blocked) {
          timestamp = 0;
        }
        blockingLog(std::cout, "+\trook[", num, "]: move{", step,
                    "} from (", x, ",", y, ") to (", new_x, ",", new_y, ")");
        move(new_x, new_y);
      }
    }
  }

  /**
  * @brief Generates next unique step position
  */
  std::pair<int, int> generateNextStep() const {
    int new_x, new_y;
    do {
      new_x = x; new_y = y;
      if (getRandomInt<0,1>()) {
        new_x = getRandomInt();
      } else {
        new_y = getRandomInt();
      }
    } while (posIsTaken(new_x, new_y));
    return std::make_pair(new_x, new_y);
  }

  /**
  * @brief Generates default unique position
  */
  std::pair<int, int> generateDefaultPos() const {
    int x,y;
    do {
      x = getRandomInt();
      y = getRandomInt();
    } while (posIsTaken(x, y));
    return std::make_pair(x,y);
  }

  /**
   * @brief Moves current rook position
   */
  void move(int x_, int y_) {
    x = x_;
    y = y_;
  }

  bool isBlocked(const time_t& timestamp) {
    return timestamp != 0 && std::time(nullptr) - timestamp < collision_delay_sec;
  }

  const int num;
  std::atomic_int x;
  std::atomic_int y;
  std::thread runnable;
  static int st_num;
};

int Rook::st_num = 0;
/**@{*/

/**
* @brief  Checks if way collides with other rook
*/
bool onTheMoveWay(const Rook& rook, int new_x, int new_y) {
  auto it = rooks.cend();
  if (rook.x != new_x) {
    it = std::find_if(rooks.cbegin(), rooks.cend(), [&](const Rook& other) {
      return (rook.y == other.y) && ((rook.x < new_x) ? (rook.x < other.x && other.x <= new_x)
                                            : (rook.x > other.x && other.x >= new_x));
    });
  } else /*if (rook.y != new_y)*/ {
    it = std::find_if(rooks.cbegin(), rooks.cend(), [&](const Rook& other) {
      return (rook.x == other.x) && ((rook.y < new_y) ? (rook.y < other.y && other.y <= new_y)
                                            : (rook.y > other.y && other.y >= new_y));
    });
  }
  if (it != rooks.cend()) {
    blockingLog(std::cerr, "-\trook[", rook.num, "]: failed to move from (", rook.x, ",", rook.y, ") to (",
                new_x, ",", new_y, ") - collides Rook[", it->num, "] at (", it->x, ",", it->y, ")");
    return true;
  }
  return false;
}

/**
* @brief  Checks if postion is already taken
*/
bool posIsTaken(int x, int y) {
  auto it = std::find_if(rooks.cbegin(), rooks.cend(), [x, y](const Rook& other){
    return other.x == x && other.y == y;
  });
  return it != rooks.cend();
}

int main() {
  auto rook_count = getRandomInt<rook_count_min, rook_count_max>();
  std::cout << "====\tInit " << rook_count <<" rooks on the field\t====" << std::endl;
  rooks = std::vector<Rook>(static_cast<size_t >(rook_count));

  std::cout << "====\tWait 1 seconds for start\t====" << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  start = true;
  cv_move.notify_all();
}