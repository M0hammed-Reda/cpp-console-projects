#include <bits/stdc++.h>
using namespace std;

class Patient {
private:
    string name = "";
    int age = 0;
    string phone_number = "";
    string address = "";
    bool statu = 0;

public:
    Patient(string name, int age, string phone_number, string address, bool statu)
        : name(name), age(age), phone_number(phone_number), address(address), statu(statu) {}

    string get_name() const {
        return name;
    }

    int get_age() const {
        return age;
    }

    string get_phone_number() const {
        return phone_number;
    }

    string get_address() const {
        return address;
    }

    const char* get_statu() const {
        return statu == 0 ? "Regular" : "Urgent";
    }
};

class HospitalSystem {
private:
    map<int, deque<Patient>> specialization;

    void print_separator() const {
        cout << "\n=======================================================\n";
    }

public:
    void add_new_patient() {
        print_separator();
        cout << "\nEnter Patient Details:\n";

        cout << "Name: ";
        string name;
        cin >> name;

        cout << "Age: ";
        int age;
        cin >> age;

        cout << "Phone: ";
        string phone;
        cin >> phone;

        cout << "Address: ";
        string address;
        cin >> address;

        cout << "Status (0 for Regular, 1 for Urgent): ";
        bool statu;
        cin >> statu;

        Patient patient(name, age, phone, address, statu);

        cout << "Specialization: ";
        int s;
        cin >> s;

        if ((int)specialization[s].size() < 5) {
            (statu == 0) ? specialization[s].push_front(patient) : specialization[s].push_back(patient);
            cout << "\nPatient added successfully!\n";
        }
        else {
            cout << "\nSorry, we can't add more than 5 patients in this specialization.\n";
        }
    }

    void print_all_patient() {
        print_separator();
        cout << "\nAll Patients in the System:\n";

        for (auto& pair : specialization) {
            cout << "\nSpecialization " << pair.first << " (" << pair.second.size() << " patients):\n";

            for (int i = 0; i < (int)pair.second.size(); ++i) {
                cout << "  " << i + 1 << ") Name: " << pair.second[i].get_name()
                    << ", Age: " << pair.second[i].get_age()
                    << ", Status: " << pair.second[i].get_statu() << "\n";
            }
        }
    }

    void get_next_patient() {
        print_separator();
        cout << "\nEnter Specialization: ";
        int s;
        cin >> s;

        if (specialization[s].empty()) {
            cout << "\nNo patients at the moment. Have a rest, Dr. Mohamed Reda.\n";
        }
        else {
            cout << "\nNext Patient: " << specialization[s][0].get_name() << ". Please proceed to Dr. Mohamed Reda.\n";
            specialization[s].pop_front();
        }
    }

    void run() {
        while (true) {
            print_separator();
            cout << "\nMenu:\n";
            cout << "  1) Add New Patient\n";
            cout << "  2) Print All Patients\n";
            cout << "  3) Get Next Patient\n";
            cout << "  4) Exit\n";
            cout << "\nEnter your choice: ";

            int choice;
            cin >> choice;

            switch (choice) {
            case 1:
                add_new_patient();
                break;
            case 2:
                print_all_patient();
                break;
            case 3:
                get_next_patient();
                break;
            case 4:
                cout << "\nExiting the system. Goodbye!\n";
                exit(0);
            default:
                cout << "\nInvalid choice! Please enter a number between 1 and 4.\n";
                break;
            }
        }
    }
};

int main() {
    HospitalSystem hospital;
    hospital.run();

    return 0;
}
