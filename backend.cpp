#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <map>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstring>
#include <cctype>
#include <cstdlib>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

using namespace std;

// ==================== DATA STRUCTURES ====================
class TimeSlot {
public:
    int startHour;
    int endHour;
    string day;
    
    TimeSlot(int start = 9, int end = 10, string d = "Monday") {
        startHour = start;
        endHour = end;
        day = d;
    }
    
    bool overlaps(const TimeSlot& other) const {
        if (day != other.day) return false;
        return (startHour < other.endHour && endHour > other.startHour);
    }
    
    string toString() const {
        return day + " " + to_string(startHour) + ":00-" + to_string(endHour) + ":00";
    }
};

class Classroom {
public:
    int id;
    string roomName;
    int capacity;
    vector<pair<string, TimeSlot>> bookedSlots;
    
    Classroom(string name = "", int cap = 0) : roomName(name), capacity(cap), id(-1) {}
    
    bool isAvailable(const TimeSlot& slot, string& conflictWith) {
        for (auto& booking : bookedSlots) {
            if (booking.second.overlaps(slot)) {
                conflictWith = booking.first;
                return false;
            }
        }
        return true;
    }
    
    void bookSlot(const string& subject, const TimeSlot& slot) {
        bookedSlots.push_back({subject, slot});
    }
};

struct Subject {
    int id;
    string name;
    int duration;
    int studentCount;
    string teacher;
    
    Subject(string n = "", int d = 0, int s = 0, string t = "") 
        : name(n), duration(d), studentCount(s), teacher(t), id(-1) {}
};

struct Booking {
    int id;
    int classroomId;
    string subjectName;
    string teacher;
    string day;
    int startHour;
    int endHour;
    string bookingType;
};

// ==================== STORAGE MANAGER ====================
class StorageManager {
private:
    string dataFile;
    
public:
    StorageManager() {
        dataFile = "classroom_data.txt";
    }
    
    void saveData(const vector<Classroom>& classrooms, const vector<Subject>& subjects, const vector<Booking>& bookings) {
        ofstream file(dataFile);
        if (!file.is_open()) {
            cout << "Warning: Could not save data to file\n";
            return;
        }
        
        file << "[CLASSROOMS]\n";
        for (const auto& c : classrooms) {
            file << c.id << "," << c.roomName << "," << c.capacity << "\n";
        }
        
        file << "[SUBJECTS]\n";
        for (const auto& s : subjects) {
            file << s.id << "," << s.name << "," << s.duration << "," << s.studentCount << "," << s.teacher << "\n";
        }
        
        file << "[BOOKINGS]\n";
        for (const auto& b : bookings) {
            file << b.id << "," << b.classroomId << "," << b.subjectName << "," 
                 << b.teacher << "," << b.day << "," << b.startHour << "," 
                 << b.endHour << "," << b.bookingType << "\n";
        }
        
        file.close();
        cout << "Data saved to " << dataFile << "\n";
    }
    
    void loadData(vector<Classroom>& classrooms, vector<Subject>& subjects, vector<Booking>& bookings) {
        ifstream file(dataFile);
        if (!file.is_open()) {
            cout << "No existing data file found. Using default data.\n";
            return;
        }
        
        classrooms.clear();
        subjects.clear();
        bookings.clear();
        
        string line;
        string section;
        
        while (getline(file, line)) {
            if (line == "[CLASSROOMS]") {
                section = "classrooms";
            } else if (line == "[SUBJECTS]") {
                section = "subjects";
            } else if (line == "[BOOKINGS]") {
                section = "bookings";
            } else if (!line.empty() && line[0] != '[') {
                stringstream ss(line);
                string token;
                vector<string> tokens;
                
                while (getline(ss, token, ',')) {
                    tokens.push_back(token);
                }
                
                if (section == "classrooms" && tokens.size() >= 3) {
                    Classroom c;
                    c.id = stoi(tokens[0]);
                    c.roomName = tokens[1];
                    c.capacity = stoi(tokens[2]);
                    classrooms.push_back(c);
                } else if (section == "subjects" && tokens.size() >= 5) {
                    Subject s;
                    s.id = stoi(tokens[0]);
                    s.name = tokens[1];
                    s.duration = stoi(tokens[2]);
                    s.studentCount = stoi(tokens[3]);
                    s.teacher = tokens[4];
                    subjects.push_back(s);
                } else if (section == "bookings" && tokens.size() >= 8) {
                    Booking b;
                    b.id = stoi(tokens[0]);
                    b.classroomId = stoi(tokens[1]);
                    b.subjectName = tokens[2];
                    b.teacher = tokens[3];
                    b.day = tokens[4];
                    b.startHour = stoi(tokens[5]);
                    b.endHour = stoi(tokens[6]);
                    b.bookingType = tokens[7];
                    bookings.push_back(b);
                }
            }
        }
        
        file.close();
        cout << "Data loaded from " << dataFile << "\n";
    }
};

// ==================== MAIN SYSTEM ====================
class ClassroomAllocationSystem {
private:
    vector<Classroom> classrooms;
    vector<Subject> subjects;
    vector<Booking> bookings;
    StorageManager storage;
    vector<string> days = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
    int workingStartHour = 9;
    int workingEndHour = 17;
    int nextClassroomId = 1;
    int nextSubjectId = 1;
    int nextBookingId = 1;
    
    Classroom* findBestClassroom(int requiredCapacity, const TimeSlot& slot, string& subjectName) {
        vector<Classroom*> available;
        
        for (auto& room : classrooms) {
            string conflictWith;
            if (room.capacity >= requiredCapacity && room.isAvailable(slot, conflictWith)) {
                available.push_back(&room);
            }
        }
        
        if (available.empty()) return nullptr;
        
        sort(available.begin(), available.end(), [](Classroom* a, Classroom* b) {
            return a->capacity < b->capacity;
        });
        
        return available[0];
    }
    
    bool isRoomAvailable(int roomId, const TimeSlot& slot, string& conflictWith) {
        for (const auto& booking : bookings) {
            if (booking.classroomId == roomId) {
                TimeSlot bookedSlot(booking.startHour, booking.endHour, booking.day);
                if (bookedSlot.overlaps(slot)) {
                    conflictWith = booking.subjectName;
                    return false;
                }
            }
        }
        return true;
    }
    
    void updateIds() {
        for (const auto& c : classrooms) {
            if (c.id >= nextClassroomId) nextClassroomId = c.id + 1;
        }
        for (const auto& s : subjects) {
            if (s.id >= nextSubjectId) nextSubjectId = s.id + 1;
        }
        for (const auto& b : bookings) {
            if (b.id >= nextBookingId) nextBookingId = b.id + 1;
        }
    }

    bool isValidDay(const string& day) const {
        for (const auto& d : days) {
            if (d == day) return true;
        }
        return false;
    }
    
public:
    ClassroomAllocationSystem() {
        initializeDefaultData();
        storage.loadData(classrooms, subjects, bookings);
        updateIds();
        
        // Rebuild booked slots from bookings
        for (auto& room : classrooms) {
            room.bookedSlots.clear();
            for (const auto& booking : bookings) {
                if (booking.classroomId == room.id) {
                    TimeSlot slot(booking.startHour, booking.endHour, booking.day);
                    room.bookSlot(booking.subjectName, slot);
                }
            }
        }
    }

    const vector<Booking>& getBookings() const {
        return bookings;
    }
    
    void initializeDefaultData() {
        if (classrooms.empty()) {
            classrooms.push_back(Classroom("Room 101", 30));
            classrooms.push_back(Classroom("Room 102", 40));
            classrooms.push_back(Classroom("Room 103", 50));
            classrooms.push_back(Classroom("Room 201", 60));
            classrooms.push_back(Classroom("Room 202", 80));
            classrooms.push_back(Classroom("Auditorium", 150));
            
            for (size_t i = 0; i < classrooms.size(); i++) {
                classrooms[i].id = i + 1;
            }
            nextClassroomId = classrooms.size() + 1;
        }
        
        if (subjects.empty()) {
            subjects.push_back(Subject("Mathematics", 2, 45, "Dr. Smith"));
            subjects.push_back(Subject("Physics", 2, 50, "Prof. Johnson"));
            subjects.push_back(Subject("Computer Science", 3, 55, "Dr. Williams"));
            subjects.push_back(Subject("English Literature", 1, 35, "Prof. Brown"));
            subjects.push_back(Subject("Chemistry", 2, 40, "Dr. Davis"));
            
            for (size_t i = 0; i < subjects.size(); i++) {
                subjects[i].id = i + 1;
            }
            nextSubjectId = subjects.size() + 1;
        }
    }
    
    void addClassroom(string name, int capacity) {
        Classroom c(name, capacity);
        c.id = nextClassroomId++;
        classrooms.push_back(c);
        storage.saveData(classrooms, subjects, bookings);
        cout << "Classroom " << name << " added successfully! (ID: " << c.id << ")\n";
    }
    
    void addSubject(string name, int duration, int students, string teacher) {
        Subject s(name, duration, students, teacher);
        s.id = nextSubjectId++;
        subjects.push_back(s);
        storage.saveData(classrooms, subjects, bookings);
        cout << "Subject " << name << " added successfully! (ID: " << s.id << ")\n";
    }
    
    void generateTimetable() {
        cout << "\n========================================\n";
        cout << "GENERATING COURSE TIMETABLE\n";
        cout << "========================================\n";
        
        // Clear existing bookings
        bookings.clear();
        for (auto& room : classrooms) {
            room.bookedSlots.clear();
        }
        
        // Sort subjects by student count descending
        vector<Subject> sortedSubjects = subjects;
        sort(sortedSubjects.begin(), sortedSubjects.end(), [](const Subject& a, const Subject& b) {
            return a.studentCount > b.studentCount;
        });
        
        int scheduled = 0;
        for (auto& subject : sortedSubjects) {
            bool scheduled_flag = false;
            
            for (string& day : days) {
                if (scheduled_flag) break;
                
                for (int hour = workingStartHour; hour + subject.duration <= workingEndHour; hour++) {
                    TimeSlot slot(hour, hour + subject.duration, day);
                    string conflictWith;
                    
                    Classroom* bestRoom = findBestClassroom(subject.studentCount, slot, subject.name);
                    
                    if (bestRoom != nullptr) {
                        Booking b;
                        b.id = nextBookingId++;
                        b.classroomId = bestRoom->id;
                        b.subjectName = subject.name;
                        b.teacher = subject.teacher;
                        b.day = day;
                        b.startHour = hour;
                        b.endHour = hour + subject.duration;
                        b.bookingType = "regular";
                        
                        bookings.push_back(b);
                        bestRoom->bookSlot(subject.name, slot);
                        
                        cout << "Scheduled: " << subject.name << " | Room: " << bestRoom->roomName 
                             << " | " << day << " " << hour << ":00 - " << hour + subject.duration << ":00";
                        cout << " | Teacher: " << subject.teacher << " | Students: " << subject.studentCount << endl;
                        scheduled_flag = true;
                        scheduled++;
                        break;
                    }
                }
            }
            
            if (!scheduled_flag) {
                cout << "FAILED to schedule: " << subject.name << " (No available classroom/time)\n";
            }
        }
        
        cout << "\nTimetable generation complete! Scheduled " << scheduled << "/" << subjects.size() << " subjects.\n";
        storage.saveData(classrooms, subjects, bookings);
    }

    bool allocateExtraClassApi(const string& teacherName, int numStudents, int duration, const string& day, int startHour, string& errorMessage) {
        if (teacherName.empty()) {
            errorMessage = "Teacher name is required";
            return false;
        }
        if (!isValidDay(day)) {
            errorMessage = "Invalid day";
            return false;
        }
        if (numStudents <= 0) {
            errorMessage = "Invalid number of students";
            return false;
        }
        if (duration <= 0) {
            errorMessage = "Invalid duration";
            return false;
        }
        if (startHour < workingStartHour || startHour + duration > workingEndHour) {
            errorMessage = "Time slot outside working hours";
            return false;
        }

        TimeSlot requestedSlot(startHour, startHour + duration, day);
        vector<Classroom*> availableRooms;

        for (auto& room : classrooms) {
            string conflictWith;
            if (room.capacity >= numStudents && isRoomAvailable(room.id, requestedSlot, conflictWith)) {
                availableRooms.push_back(&room);
            }
        }

        if (availableRooms.empty()) {
            errorMessage = "No available classrooms for this time slot";
            return false;
        }

        sort(availableRooms.begin(), availableRooms.end(), [](Classroom* a, Classroom* b) {
            return a->capacity < b->capacity;
        });

        Classroom* chosenRoom = availableRooms[0];

        Booking b;
        b.id = nextBookingId++;
        b.classroomId = chosenRoom->id;
        b.subjectName = "EXTRA: " + teacherName;
        b.teacher = teacherName;
        b.day = day;
        b.startHour = startHour;
        b.endHour = startHour + duration;
        b.bookingType = "extra";

        bookings.push_back(b);
        chosenRoom->bookSlot(b.subjectName, requestedSlot);
        storage.saveData(classrooms, subjects, bookings);
        return true;
    }
    
    void allocateExtraClass() {
        cout << "\n========================================\n";
        cout << "EXTRA CLASS ALLOCATION (TEACHER ACCESS)\n";
        cout << "========================================\n";
        
        string teacherName;
        int numStudents;
        int duration;
        string day;
        int startHour;
        
        cin.ignore();
        cout << "Enter teacher name: ";
        getline(cin, teacherName);
        cout << "Enter number of students: ";
        cin >> numStudents;
        cout << "Enter duration (hours): ";
        cin >> duration;
        cout << "Enter day (Monday/Tuesday/Wednesday/Thursday/Friday): ";
        cin >> day;
        cout << "Enter start hour (9-16): ";
        cin >> startHour;
        
        // Validate day
        bool validDay = false;
        for (const string& d : days) {
            if (d == day) validDay = true;
        }
        if (!validDay) {
            cout << "Invalid day! Please use Monday, Tuesday, Wednesday, Thursday, or Friday.\n";
            return;
        }
        
        if (startHour < workingStartHour || startHour + duration > workingEndHour) {
            cout << "Error: Time slot outside working hours (9:00 - 17:00)\n";
            return;
        }
        
        TimeSlot requestedSlot(startHour, startHour + duration, day);
        
        vector<Classroom*> availableRooms;
        
        for (auto& room : classrooms) {
            string conflictWith;
            if (room.capacity >= numStudents && isRoomAvailable(room.id, requestedSlot, conflictWith)) {
                availableRooms.push_back(&room);
            }
        }
        
        if (availableRooms.empty()) {
            cout << "\nNo available classrooms for this time slot!\n";
            cout << "Suggested alternative times:\n";
            
            int suggestions = 0;
            for (string& altDay : days) {
                if (suggestions >= 5) break;
                if (altDay == day) continue;
                for (int altHour = workingStartHour; altHour + duration <= workingEndHour; altHour++) {
                    TimeSlot altSlot(altHour, altHour + duration, altDay);
                    for (auto& room : classrooms) {
                        string conflictWith;
                        if (room.capacity >= numStudents && isRoomAvailable(room.id, altSlot, conflictWith)) {
                            cout << "  -> " << altDay << " " << altHour << ":00 - " << altHour + duration << ":00 in " << room.roomName << endl;
                            suggestions++;
                            break;
                        }
                    }
                    if (suggestions >= 5) break;
                }
            }
            return;
        }
        
        sort(availableRooms.begin(), availableRooms.end(), [](Classroom* a, Classroom* b) {
            return a->capacity < b->capacity;
        });
        
        Classroom* chosenRoom = availableRooms[0];
        
        Booking b;
        b.id = nextBookingId++;
        b.classroomId = chosenRoom->id;
        b.subjectName = "EXTRA: " + teacherName;
        b.teacher = teacherName;
        b.day = day;
        b.startHour = startHour;
        b.endHour = startHour + duration;
        b.bookingType = "extra";
        
        bookings.push_back(b);
        chosenRoom->bookSlot(b.subjectName, requestedSlot);
        storage.saveData(classrooms, subjects, bookings);
        
        cout << "\nExtra class allocated successfully!\n";
        cout << "  Classroom: " << chosenRoom->roomName << " (Capacity: " << chosenRoom->capacity << ")\n";
        cout << "  Time: " << day << " " << startHour << ":00 - " << startHour + duration << ":00\n";
        cout << "  Teacher: " << teacherName << "\n";
        cout << "  Booking ID: " << b.id << "\n";
    }
    
    void displayAllSchedules() {
        cout << "\n========================================\n";
        cout << "COMPLETE CLASSROOM SCHEDULES\n";
        cout << "========================================\n";
        
        if (bookings.empty()) {
            cout << "No classes scheduled yet. Generate timetable first!\n";
            return;
        }
        
        for (auto& room : classrooms) {
            cout << "\n+---------------------------------------+\n";
            cout << "| " << setw(30) << left << room.roomName << " |\n";
            cout << "| Capacity: " << setw(21) << left << room.capacity << " |\n";
            cout << "+---------------------------------------+\n";
            
            bool hasBookings = false;
            for (auto& booking : bookings) {
                if (booking.classroomId == room.id) {
                    cout << "| " << booking.subjectName << "\n";
                    cout << "|   " << booking.day << " " << booking.startHour << ":00-" << booking.endHour << ":00\n";
                    cout << "|   Teacher: " << booking.teacher << "\n";
                    cout << "|   Type: " << booking.bookingType << "\n";
                    cout << "+---------------------------------------+\n";
                    hasBookings = true;
                }
            }
            if (!hasBookings) {
                cout << "| No classes booked                          |\n";
                cout << "+---------------------------------------+\n";
            }
        }
    }
    
    void viewAvailableClassrooms() {
        cout << "\n========================================\n";
        cout << "AVAILABLE CLASSROOMS\n";
        cout << "========================================\n";
        
        for (auto& room : classrooms) {
            int bookingCount = 0;
            for (auto& booking : bookings) {
                if (booking.classroomId == room.id) bookingCount++;
            }
            cout << "• " << room.roomName << " - Capacity: " << room.capacity 
                 << " - Booked: " << bookingCount << " classes\n";
        }
    }
    
    void viewAllBookings() {
        cout << "\n========================================\n";
        cout << "ALL BOOKINGS\n";
        cout << "========================================\n";
        
        if (bookings.empty()) {
            cout << "No bookings found.\n";
            return;
        }
        
        for (const auto& b : bookings) {
            cout << "ID: " << b.id << " | " << b.subjectName << " | " 
                 << b.teacher << " | " << b.day << " " << b.startHour << ":00-" << b.endHour << ":00 | " << b.bookingType << "\n";
        }
    }
    
    void exportToCSV() {
        string filename = "timetable_export.csv";
        ofstream file(filename);
        if (!file.is_open()) {
            cout << "Error: Cannot create CSV file!\n";
            return;
        }
        
        file << "Booking ID,Classroom ID,Subject,Teacher,Day,Start Hour,End Hour,Type\n";
        for (const auto& b : bookings) {
            file << b.id << "," << b.classroomId << "," << b.subjectName << ","
                 << b.teacher << "," << b.day << "," << b.startHour << ","
                 << b.endHour << "," << b.bookingType << "\n";
        }
        
        file.close();
        cout << "Data exported to " << filename << " successfully!\n";
    }
    
    void generateReport() {
        string filename = "system_report.txt";
        ofstream file(filename);
        if (!file.is_open()) {
            cout << "Error: Cannot create report!\n";
            return;
        }
        
        time_t now = time(0);
        char* dt = ctime(&now);
        
        file << "========================================\n";
        file << "CLASSROOM ALLOCATION SYSTEM REPORT\n";
        file << "Generated: " << dt;
        file << "========================================\n\n";
        
        file << "CLASSROOMS SUMMARY\n";
        file << "------------------\n";
        for (const auto& c : classrooms) {
            int bookingCount = 0;
            for (const auto& b : bookings) {
                if (b.classroomId == c.id) bookingCount++;
            }
            file << "• " << c.roomName << " (Capacity: " << c.capacity 
                 << ") - " << bookingCount << " bookings\n";
        }
        
        file << "\nBOOKINGS DETAILS\n";
        file << "----------------\n";
        for (const auto& b : bookings) {
            file << "• " << b.subjectName << " by " << b.teacher 
                 << " | " << b.day << " " << b.startHour << ":00-" << b.endHour << ":00"
                 << " | Type: " << b.bookingType << "\n";
        }
        
        file << "\nSTATISTICS\n";
        file << "----------\n";
        file << "Total Classrooms: " << classrooms.size() << "\n";
        file << "Total Subjects: " << subjects.size() << "\n";
        file << "Total Bookings: " << bookings.size() << "\n";
        
        int regularCount = 0, extraCount = 0;
        for (const auto& b : bookings) {
            if (b.bookingType == "regular") regularCount++;
            else extraCount++;
        }
        file << "Regular Classes: " << regularCount << "\n";
        file << "Extra Classes: " << extraCount << "\n";
        
        file.close();
        cout << "Report generated: " << filename << endl;
    }
    
    void clearAllData() {
        cout << "\nWARNING: This will delete ALL data!\n";
        cout << "Type 'CONFIRM' to proceed: ";
        string confirm;
        cin >> confirm;
        
        if (confirm == "CONFIRM") {
            classrooms.clear();
            subjects.clear();
            bookings.clear();
            initializeDefaultData();
            nextClassroomId = classrooms.size() + 1;
            nextSubjectId = subjects.size() + 1;
            nextBookingId = 1;
            storage.saveData(classrooms, subjects, bookings);
            cout << "All data cleared and reset to default!\n";
        } else {
            cout << "Operation cancelled.\n";
        }
    }
    
    void showMenu() {
        cout << "\n========================================\n";
        cout << "     CLASSROOM ALLOCATION SYSTEM        \n";
        cout << "     Design & Analysis of Algorithms    \n";
        cout << "========================================\n";
        cout << "  1. Generate Course Timetable         \n";
        cout << "  2. Allocate Extra Class              \n";
        cout << "  3. Display All Classroom Schedules   \n";
        cout << "  4. View Available Classrooms         \n";
        cout << "  5. View All Bookings                 \n";
        cout << "  6. Add New Classroom                 \n";
        cout << "  7. Add New Subject                   \n";
        cout << "  8. Export to CSV                     \n";
        cout << "  9. Generate Report                   \n";
        cout << " 10. Clear All Data                    \n";
        cout << "  0. Exit                              \n";
        cout << "========================================\n";
        cout << "Enter choice: ";
    }
};

// ==================== SIMPLE HTTP SERVER ====================
#ifdef _WIN32
using SocketType = SOCKET;
static const SocketType kInvalidSocket = INVALID_SOCKET;
static int closeSocket(SocketType socket) { return closesocket(socket); }
#else
using SocketType = int;
static const SocketType kInvalidSocket = -1;
static int closeSocket(SocketType socket) { return close(socket); }
#endif

struct HttpRequest {
    string method;
    string path;
    string body;
    map<string, string> headers;
};

static string ltrim(const string& value) {
    size_t start = 0;
    while (start < value.size() && isspace(static_cast<unsigned char>(value[start]))) {
        start++;
    }
    return value.substr(start);
}

static string rtrim(const string& value) {
    size_t end = value.size();
    while (end > 0 && isspace(static_cast<unsigned char>(value[end - 1]))) {
        end--;
    }
    return value.substr(0, end);
}

static string trim(const string& value) {
    return rtrim(ltrim(value));
}

static string toLower(const string& value) {
    string out = value;
    transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(tolower(c));
    });
    return out;
}

static string jsonEscape(const string& input) {
    string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

static bool readFileToString(const string& path, string& out) {
    ifstream file(path, ios::binary);
    if (!file.is_open()) return false;
    ostringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

static int parseContentLength(const string& headerPart) {
    istringstream stream(headerPart);
    string line;
    while (getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto colon = line.find(':');
        if (colon == string::npos) continue;
        string key = toLower(trim(line.substr(0, colon)));
        if (key == "content-length") {
            string value = trim(line.substr(colon + 1));
            if (!value.empty()) {
                return stoi(value);
            }
        }
    }
    return 0;
}

static bool readHttpRequest(SocketType client, string& raw) {
    string data;
    char buffer[4096];
    size_t headerEnd = string::npos;
    int contentLength = 0;

    while (true) {
        int received = recv(client, buffer, sizeof(buffer), 0);
        if (received <= 0) break;
        data.append(buffer, received);
        if (headerEnd == string::npos) {
            headerEnd = data.find("\r\n\r\n");
            if (headerEnd != string::npos) {
                contentLength = parseContentLength(data.substr(0, headerEnd));
            }
        }
        if (headerEnd != string::npos) {
            size_t bodyStart = headerEnd + 4;
            if (data.size() >= bodyStart + static_cast<size_t>(contentLength)) {
                break;
            }
        }
    }

    if (data.empty()) return false;
    raw = data;
    return true;
}

static bool parseHttpRequest(const string& raw, HttpRequest& request) {
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == string::npos) return false;

    string headerPart = raw.substr(0, headerEnd);
    request.body = raw.substr(headerEnd + 4);

    istringstream headerStream(headerPart);
    string requestLine;
    if (!getline(headerStream, requestLine)) return false;
    if (!requestLine.empty() && requestLine.back() == '\r') requestLine.pop_back();

    istringstream requestLineStream(requestLine);
    requestLineStream >> request.method >> request.path;
    if (request.method.empty() || request.path.empty()) return false;

    string line;
    while (getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto colon = line.find(':');
        if (colon == string::npos) continue;
        string key = toLower(trim(line.substr(0, colon)));
        string value = trim(line.substr(colon + 1));
        request.headers[key] = value;
    }

    size_t queryPos = request.path.find('?');
    if (queryPos != string::npos) {
        request.path = request.path.substr(0, queryPos);
    }
    return true;
}

static string statusText(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

static string buildResponse(int statusCode, const string& contentType, const string& body, const map<string, string>& extraHeaders) {
    ostringstream out;
    out << "HTTP/1.1 " << statusCode << " " << statusText(statusCode) << "\r\n";
    if (!contentType.empty()) {
        out << "Content-Type: " << contentType << "\r\n";
    }
    out << "Content-Length: " << body.size() << "\r\n";
    out << "Connection: close\r\n";
    for (const auto& header : extraHeaders) {
        out << header.first << ": " << header.second << "\r\n";
    }
    out << "\r\n";
    out << body;
    return out.str();
}

static map<string, string> defaultHeaders() {
    map<string, string> headers;
    headers["Access-Control-Allow-Origin"] = "*";
    headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    headers["Access-Control-Allow-Headers"] = "Content-Type";
    return headers;
}

static string bookingsToJson(const vector<Booking>& bookings) {
    ostringstream out;
    out << "{\"bookings\":[";
    for (size_t i = 0; i < bookings.size(); ++i) {
        const auto& b = bookings[i];
        if (i > 0) out << ",";
        out << "{"
            << "\"id\":" << b.id << ","
            << "\"classroomId\":" << b.classroomId << ","
            << "\"subject\":\"" << jsonEscape(b.subjectName) << "\","
            << "\"teacher\":\"" << jsonEscape(b.teacher) << "\","
            << "\"day\":\"" << jsonEscape(b.day) << "\","
            << "\"startHour\":" << b.startHour << ","
            << "\"endHour\":" << b.endHour << ","
            << "\"type\":\"" << jsonEscape(b.bookingType) << "\""
            << "}";
    }
    out << "]}";
    return out.str();
}

static bool getJsonString(const string& json, const string& key, string& value) {
    string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == string::npos) return false;
    pos = json.find(':', pos + pattern.size());
    if (pos == string::npos) return false;
    pos++;
    while (pos < json.size() && isspace(static_cast<unsigned char>(json[pos]))) pos++;
    if (pos >= json.size() || json[pos] != '"') return false;
    pos++;
    string result;
    while (pos < json.size()) {
        char c = json[pos];
        if (c == '\\') {
            if (pos + 1 < json.size()) {
                result.push_back(json[pos + 1]);
                pos += 2;
                continue;
            }
            break;
        }
        if (c == '"') {
            value = result;
            return true;
        }
        result.push_back(c);
        pos++;
    }
    return false;
}

static bool getJsonInt(const string& json, const string& key, int& value) {
    string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == string::npos) return false;
    pos = json.find(':', pos + pattern.size());
    if (pos == string::npos) return false;
    pos++;
    while (pos < json.size() && isspace(static_cast<unsigned char>(json[pos]))) pos++;
    const char* start = json.c_str() + pos;
    char* end = nullptr;
    long parsed = strtol(start, &end, 10);
    if (start == end) return false;
    value = static_cast<int>(parsed);
    return true;
}

static string handleRequest(const HttpRequest& request, ClassroomAllocationSystem& system) {
    map<string, string> headers = defaultHeaders();

    if (request.method == "OPTIONS") {
        return buildResponse(204, "text/plain", "", headers);
    }

    if (request.path == "/" || request.path == "/frontend.html") {
        string html;
        if (readFileToString("frontend.html", html)) {
            return buildResponse(200, "text/html; charset=utf-8", html, headers);
        }
        return buildResponse(404, "text/plain", "frontend.html not found", headers);
    }

    if (request.path.rfind("/api/", 0) == 0) {
        headers["Cache-Control"] = "no-store";
    }

    if (request.path == "/api/bookings") {
        if (request.method != "GET") {
            return buildResponse(405, "application/json", "{\"error\":\"Method not allowed\"}", headers);
        }
        string body = bookingsToJson(system.getBookings());
        return buildResponse(200, "application/json", body, headers);
    }

    if (request.path == "/api/generate") {
        if (request.method != "POST") {
            return buildResponse(405, "application/json", "{\"error\":\"Method not allowed\"}", headers);
        }
        system.generateTimetable();
        string body = bookingsToJson(system.getBookings());
        return buildResponse(200, "application/json", body, headers);
    }

    if (request.path == "/api/extra") {
        if (request.method != "POST") {
            return buildResponse(405, "application/json", "{\"error\":\"Method not allowed\"}", headers);
        }

        string teacherName;
        string day;
        int numStudents = 0;
        int duration = 0;
        int startHour = 0;

        bool ok = getJsonString(request.body, "teacherName", teacherName)
            && getJsonInt(request.body, "numStudents", numStudents)
            && getJsonInt(request.body, "duration", duration)
            && getJsonString(request.body, "day", day)
            && getJsonInt(request.body, "startHour", startHour);

        if (!ok) {
            return buildResponse(400, "application/json", "{\"error\":\"Invalid request body\"}", headers);
        }

        string errorMessage;
        if (!system.allocateExtraClassApi(teacherName, numStudents, duration, day, startHour, errorMessage)) {
            string body = "{\"error\":\"" + jsonEscape(errorMessage) + "\"}";
            return buildResponse(409, "application/json", body, headers);
        }

        string body = bookingsToJson(system.getBookings());
        return buildResponse(200, "application/json", body, headers);
    }

    if (request.path == "/api/export/csv") {
        if (request.method != "GET") {
            return buildResponse(405, "application/json", "{\"error\":\"Method not allowed\"}", headers);
        }
        system.exportToCSV();
        return buildResponse(200, "application/json", "{\"ok\":true}", headers);
    }

    if (request.path == "/api/report") {
        if (request.method != "GET") {
            return buildResponse(405, "application/json", "{\"error\":\"Method not allowed\"}", headers);
        }
        system.generateReport();
        return buildResponse(200, "application/json", "{\"ok\":true}", headers);
    }

    return buildResponse(404, "text/plain", "Not found", headers);
}

static bool sendAll(SocketType client, const string& data) {
    size_t total = 0;
    while (total < data.size()) {
        int sent = send(client, data.data() + total, static_cast<int>(data.size() - total), 0);
        if (sent <= 0) return false;
        total += static_cast<size_t>(sent);
    }
    return true;
}

static bool initSockets() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

static void cleanupSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// ==================== MAIN FUNCTION ====================
static int runCli(ClassroomAllocationSystem& system) {
    int choice;

    cout << "\n";
    cout << "========================================\n";
    cout << "                                     \n";
    cout << "     AUTOMATIC CLASSROOM ALLOCATION     \n";
    cout << "     SYSTEM                          \n";
    cout << "                                     \n";
    cout << "     Design & Analysis of Algorithms   \n";
    cout << "     Project                         \n";
    cout << "                                     \n";
    cout << "========================================\n";

    do {
        system.showMenu();
        cin >> choice;

        switch(choice) {
            case 1:
                system.generateTimetable();
                break;
            case 2:
                system.allocateExtraClass();
                break;
            case 3:
                system.displayAllSchedules();
                break;
            case 4:
                system.viewAvailableClassrooms();
                break;
            case 5:
                system.viewAllBookings();
                break;
            case 6: {
                string name;
                int cap;
                cout << "Enter classroom name: ";
                cin >> name;
                cout << "Enter capacity: ";
                cin >> cap;
                system.addClassroom(name, cap);
                break;
            }
            case 7: {
                string name, teacher;
                int duration, students;
                cout << "Enter subject name: ";
                cin >> name;
                cout << "Enter duration (hours): ";
                cin >> duration;
                cout << "Enter number of students: ";
                cin >> students;
                cout << "Enter teacher name: ";
                cin >> teacher;
                system.addSubject(name, duration, students, teacher);
                break;
            }
            case 8:
                system.exportToCSV();
                break;
            case 9:
                system.generateReport();
                break;
            case 10:
                system.clearAllData();
                break;
            case 0:
                cout << "\n========================================\n";
                cout << "  Thank you for using Classroom System!  \n";
                cout << "  Goodbye!                               \n";
                cout << "========================================\n";
                break;
            default:
                cout << "Invalid choice! Please try again.\n";
        }
    } while (choice != 0);

    return 0;
}

static int runServer(ClassroomAllocationSystem& system) {
    if (!initSockets()) {
        cerr << "Failed to initialize sockets.\n";
        return 1;
    }

    SocketType serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == kInvalidSocket) {
        cerr << "Failed to create server socket.\n";
        cleanupSockets();
        return 1;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) != 0) {
        cerr << "Failed to bind to port 8080.\n";
        closeSocket(serverSocket);
        cleanupSockets();
        return 1;
    }

    if (listen(serverSocket, 10) != 0) {
        cerr << "Failed to listen on server socket.\n";
        closeSocket(serverSocket);
        cleanupSockets();
        return 1;
    }

    cout << "Server running at http://localhost:8080" << endl;
    cout << "Press Ctrl+C to stop." << endl;

    while (true) {
        sockaddr_in clientAddr{};
#ifdef _WIN32
        int clientAddrLen = sizeof(clientAddr);
#else
        socklen_t clientAddrLen = sizeof(clientAddr);
#endif
        SocketType clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
        if (clientSocket == kInvalidSocket) {
            continue;
        }

        string rawRequest;
        if (!readHttpRequest(clientSocket, rawRequest)) {
            closeSocket(clientSocket);
            continue;
        }

        HttpRequest request;
        if (!parseHttpRequest(rawRequest, request)) {
            string response = buildResponse(400, "text/plain", "Bad Request", defaultHeaders());
            sendAll(clientSocket, response);
            closeSocket(clientSocket);
            continue;
        }

        string response = handleRequest(request, system);
        sendAll(clientSocket, response);
        closeSocket(clientSocket);
    }

    closeSocket(serverSocket);
    cleanupSockets();
    return 0;
}

int main(int argc, char* argv[]) {
    ClassroomAllocationSystem system;
    if (argc > 1 && string(argv[1]) == "--cli") {
        return runCli(system);
    }
    return runServer(system);
