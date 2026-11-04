// Nested definition included inside ServiceShellQS; preserves its qualified name and access.
    class CallbacksQS {
    public:
        std::function<void()> backToServices;
        std::function<void()> logoutService;
        std::function<void()> switchUser;
    };
