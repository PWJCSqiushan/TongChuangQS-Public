// Nested definition included inside ChatNodeQS; preserves its qualified name and access.
    class IdentityQS {
    public:
        std::string nickname;
        std::unique_ptr<FileLockQS> claim;
        std::unique_ptr<ChatHistoryQS> history;
    };
