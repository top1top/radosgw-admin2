#include <iostream>
#include <boost/program_options.hpp>
#include <unordered_set>

namespace po = boost::program_options;

class ProcessCmdError {
public:
    explicit ProcessCmdError(const std::string& message) : message_(message) {}

    explicit ProcessCmdError(std::string&& message) : message_(std::move(message)) {}

    explicit ProcessCmdError(const char* message) : message_(message) {}

    std::string getMessage() const {
        return message_;
    }

private:
    std::string message_;
};

template <typename CmdIterator>
void process_user_create_cmd(CmdIterator it_begin, CmdIterator it_end, po::variables_map& vm, int uid) {
    if (it_begin != it_end)
        throw ProcessCmdError("invalid command, too many positional arguments");
    if (vm.size() > 3)
        throw ProcessCmdError("too many options for this command");

    auto opt_display_name = vm["display-name"];
    if (opt_display_name.empty())
        throw ProcessCmdError("display-name is required for this command");
    auto opt_email = vm["email"];
    if (opt_email.empty())
        throw ProcessCmdError("email is required for this command");
    auto display_name = opt_display_name.as<std::string>();
    auto email = opt_email.as<std::string>();
    std::cout << "user created with uid " << uid
              << " display-name " << display_name
              << " and email " << email << std::endl;
}

template <typename CmdIterator>
void process_user_info_cmd(CmdIterator it_begin, CmdIterator it_end, po::variables_map& vm, int uid) {
    if (it_begin != it_end)
        throw ProcessCmdError("invalid command, too many positional arguments");
    if (vm.size() > 1)
        throw ProcessCmdError("too many options for this command");
    std::cout << "info about user with uid " << uid << std::endl;
}

template <typename CmdIterator>
void process_user_delete_cmd(CmdIterator it_begin, CmdIterator it_end, po::variables_map& vm, int uid) {
    if (it_begin != it_end)
        throw ProcessCmdError("invalid command, too many positional arguments");
    if (vm.size() > 1)
        throw ProcessCmdError("too many options for this command");
    std::cout << "user with uid " << uid << ' ' << " was deleted" << std::endl;
}

template <typename CmdIterator>
void process_user_cmd(CmdIterator it_begin, CmdIterator it_end, po::variables_map& vm) {
    if (it_begin == it_end)
        throw ProcessCmdError("invalid command, too few positional arguments");
    auto opt_uid = vm["uid"];
    if (opt_uid.empty())
        throw ProcessCmdError("uid is require for this command");
    vm.erase("uid");
    auto uid = opt_uid.as<int>();
    if (*it_begin == "create")
        process_user_create_cmd(std::next(it_begin), it_end, vm, uid);
    else if (*it_begin == "info")
        process_user_info_cmd(std::next(it_begin), it_end, vm, uid);
    else if (*it_begin == "delete")
        process_user_delete_cmd(std::next(it_begin), it_end, vm, uid);
    else
        throw ProcessCmdError(std::string("no such command user ") + *it_begin);
}

template <typename CmdIterator>
void process_cmd(CmdIterator it_begin, CmdIterator it_end, po::variables_map& vm) {
    if (it_begin == it_end)
        throw ProcessCmdError("invalid command, too few positional arguments");

    if (*it_begin == "user")
        process_user_cmd(std::next(it_begin), it_end, vm);
    else
        throw ProcessCmdError(std::string("no such command ") + *it_begin);
}

po::options_description register_options() {
    po::options_description desk("options");
    desk.add_options()
        ("help", "produce help message")
        ("uid", po::value<int>(), "user id")
        ("display-name", po::value<std::string>(), "")
        ("email", po::value<std::string>(), "")
    ;
    return desk;
}

void print_help(const po::options_description& ops, const char* error_message = nullptr) {
    if (error_message)
        std::cout << error_message << std::endl;
    std::cout << "usage: radosgw-admin <cmd> [options...]" << std::endl;
    std::cout << "commands:\n"
                 "user create    create a new user\n"
                 "user delete    delete a user\n"
                 "user info      get user info" << std::endl;
    ops.print(std::cout);
}

int main(int argc, char** argv) {
    po::options_description options_desc = register_options();

    const std::string command_tokens = "command_tokens";
    po::positional_options_description pos_options_desc;
    pos_options_desc.add(command_tokens.c_str(), -1);
    po::options_description options_desc_with_pos = options_desc;
    options_desc_with_pos.add_options()(command_tokens.c_str(), po::value<std::vector<std::string>>());

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(options_desc_with_pos).positional(pos_options_desc).run(), vm);
    } catch (...) {
        print_help(options_desc, "error while parse command");
        return 0;
    }
    po::notify(vm);

    if (vm.count("help")) {
        print_help(options_desc);
        return 0;
    }

    std::vector<std::string> tokens;

    try {
        tokens = vm[command_tokens].as<std::vector<std::string>>();
    } catch (const boost::bad_any_cast&) {
        print_help(options_desc, "error while parse command");
        return 0;
    }

    vm.erase(command_tokens);

    try {
        process_cmd(tokens.begin(), tokens.end(), vm);
    } catch (const ProcessCmdError& error) {
        print_help(options_desc, error.getMessage().c_str());
    }
}