#include "TestFramework.h"
#include "BoundedQueue.h"
#include "Database/DatabaseEnv.h"
#include "Database/SqlOperations.h"
#include "IO/Timer/AsyncSystemTimer.h"
#include "ThreadPool.h"
#include <atomic>
#include <chrono>
#include <future>
#include <limits>
#include <thread>
#include <stdexcept>

namespace {
using namespace std::chrono_literals;
struct Plan
{
    int created = 0, live = 0, failConnection = -1, throwConnection = -1;
    int writes = 0, begins = 0, commits = 0, rollbacks = 0;
    int failWrite = -1, preparedLive = 0;
    bool beginOk = true, commitOk = true, rollbackOk = true, deadlock = false, throwWrite = false, alwaysFail = false;
};
class FailedPrepared : public SqlPreparedStatement
{
    Plan& plan;
public:
    FailedPrepared(SqlConnection& connection, Plan& p) : SqlPreparedStatement("SELECT ?", connection), plan(p) { ++plan.preparedLive; }
    ~FailedPrepared() override { --plan.preparedLive; }
    bool prepare() override { return false; }
    void bind(SqlStmtParameters const&) override {}
    bool execute() override { return false; }
    std::unique_ptr<QueryResult> query() override { return nullptr; }
};
class FakeConnection : public SqlConnection
{
    Plan& plan;
    int index;
public:
    FakeConnection(Database& db, Plan& p) : SqlConnection(db), plan(p), index(p.created++) { ++plan.live; }
    ~FakeConnection() override { FreePreparedStatements(); --plan.live; }
    bool OpenConnection(bool) override { if (index == plan.throwConnection) throw std::runtime_error("injected connection failure"); return index != plan.failConnection; }
    std::unique_ptr<QueryResult> Query(std::string const&) override { return nullptr; }
    std::unique_ptr<QueryNamedResult> QueryNamed(std::string const&) override { return nullptr; }
    bool Execute(std::string const&) override { ++plan.writes; if (plan.throwWrite) throw std::runtime_error("injected write failure"); return !plan.alwaysFail && plan.writes != plan.failWrite; }
    bool BeginTransaction() override { ++plan.begins; return plan.beginOk; }
    bool CommitTransaction() override { ++plan.commits; return plan.commitOk; }
    bool RollbackTransaction() override { ++plan.rollbacks; return plan.rollbackOk; }
    bool CanRetryTransaction() const override { return plan.deadlock; }
protected:
    SqlPreparedStatement* CreateStatement(std::string const&) override { return new FailedPrepared(*this, plan); }
};
class FakeDatabase : public Database
{
public:
    Plan plan;
    ~FakeDatabase() override { StopServer(); m_currentTransaction.reset(); }
    void InstallConnections(unsigned count)
    {
        for (unsigned i = 0; i < count; ++i)
            m_pQueryConnections.push_back(new FakeConnection(*this, plan));
        m_nQueryConnPoolSize = count;
        m_pAsyncConn = new FakeConnection(*this, plan);
    }
    void SeedSelector(uint32 value) { m_nQueryCounter.store(value); }
    bool SelectValid()
    {
        auto* conn = getQueryConnection();
        return std::find(m_pQueryConnections.begin(), m_pQueryConnections.end(), conn) != m_pQueryConnections.end();
    }
protected:
    SqlConnection* CreateConnection() override { return new FakeConnection(*this, plan); }
};
struct AcceptAll { template <typename T> bool Process(T const&) const { return true; } };
struct RejectAll { template <typename T> bool Process(T const&) const { return false; } };
}

TEST(Transaction_FailedBeginExecutesNothing)
{
    FakeDatabase db;
    FakeConnection connection(db, db.plan);
    db.plan.beginOk = false;
    SqlTransaction tx(1);
    tx.DelayExecute(new SqlPlainRequest("write"));
    ASSERT_FALSE(tx.Execute(&connection));
    ASSERT_EQ(db.plan.writes, 0);
    ASSERT_EQ(db.plan.commits, 0);
}

TEST(Transaction_FailureRollsBackWithoutReplay)
{
    FakeDatabase db;
    FakeConnection connection(db, db.plan);
    db.plan.failWrite = 2;
    SqlTransaction tx(1);
    tx.DelayExecute(new SqlPlainRequest("first"));
    tx.DelayExecute(new SqlPlainRequest("second"));
    tx.DelayExecute(new SqlPlainRequest("third"));
    ASSERT_FALSE(tx.Execute(&connection));
    ASSERT_EQ(db.plan.writes, 2);
    ASSERT_EQ(db.plan.begins, 1);
    ASSERT_EQ(db.plan.rollbacks, 1);
    ASSERT_EQ(db.plan.commits, 0);
}

TEST(Transaction_ConfirmedDeadlockRetriesWholeTransaction)
{
    FakeDatabase db;
    FakeConnection connection(db, db.plan);
    db.plan.failWrite = 2;
    db.plan.deadlock = true;
    SqlTransaction tx(1);
    tx.DelayExecute(new SqlPlainRequest("first"));
    tx.DelayExecute(new SqlPlainRequest("second"));
    ASSERT_TRUE(tx.Execute(&connection));
    ASSERT_EQ(db.plan.writes, 4);
    ASSERT_EQ(db.plan.begins, 2);
    ASSERT_EQ(db.plan.rollbacks, 1);
    ASSERT_EQ(db.plan.commits, 1);
}

TEST(Transaction_CommitFailureIsNeverReplayed)
{
    FakeDatabase db;
    FakeConnection connection(db, db.plan);
    db.plan.commitOk = false;
    db.plan.deadlock = true;
    SqlTransaction tx(1);
    tx.DelayExecute(new SqlPlainRequest("write"));
    ASSERT_FALSE(tx.Execute(&connection));
    ASSERT_EQ(db.plan.writes, 1);
    ASSERT_EQ(db.plan.begins, 1);
    ASSERT_EQ(db.plan.rollbacks, 1);
}

TEST(Database_DirectCommitPropagatesFailure)
{
    FakeDatabase db;
    db.InstallConnections(1);
    db.plan.commitOk = false;
    ASSERT_TRUE(db.BeginTransaction());
    ASSERT_TRUE(db.Execute("write"));
    ASSERT_FALSE(db.CommitTransactionDirect());
    ASSERT_FALSE(db.InTransaction());
}

TEST(Database_FailedPrepareReleasesEveryCandidate)
{
    FakeDatabase db;
    FakeConnection connection(db, db.plan);
    SqlStatementID id;
    db.CreateStatement(id, "SELECT ?");
    SqlStmtParameters params(1);
    params.addParam(SqlStmtFieldData(uint32(1)));
    for (unsigned i = 0; i < 32; ++i)
        ASSERT_FALSE(connection.ExecuteStmt(id.ID(), params));
    ASSERT_EQ(db.plan.preparedLive, 0);
}

TEST(Database_SelectorWrapAndConcurrencyStayInPool)
{
    FakeDatabase db;
    db.InstallConnections(3);
    db.SeedSelector(std::numeric_limits<uint32>::max() - 100);
    std::atomic<bool> valid{true};
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 4; ++i)
        threads.emplace_back([&]() { for (unsigned j = 0; j < 1000; ++j) if (!db.SelectValid()) valid = false; });
    for (auto& thread : threads) thread.join();
    ASSERT_TRUE(valid.load());
}

TEST(Database_RejectsTruncatedFormattedStatements)
{
    FakeDatabase db;
    db.InstallConnections(1);
    std::string oversized(MAX_QUERY_LEN, 'x');
    ASSERT_FALSE(db.PExecute("%s", oversized.c_str()));
    ASSERT_TRUE(db.PQuery("%s", oversized.c_str()) == nullptr);
    ASSERT_EQ(db.plan.writes, 0);
    SqlQueryHolder holder;
    holder.SetSize(1);
    ASSERT_FALSE(holder.SetPQuery(0, "%s", oversized.c_str()));
}

TEST(Database_PartialWorkerStartupUnwindsCreatedConnections)
{
    DatabaseMysql mysqlLibrary;
    FakeDatabase db;
    // Exercise failure at the first pool connection, dedicated async connection,
    // and each possible worker position, including a partially live pool.
    for (int fail = 0; fail < 5; ++fail)
    {
        db.plan.created = 0;
        db.plan.failConnection = fail;
        ASSERT_FALSE(db.Initialize("localhost;3306;test;test;test", 1, 3));
        ASSERT_EQ(db.plan.live, 0);
        db.StopServer();
    }
}

TEST(PacketQueue_AdmissionAndAccounting)
{
    BoundedQueue<std::unique_ptr<int>, 2, 10> queue;
    AcceptAll accept;
    RejectAll reject;
    ASSERT_TRUE(queue.tryAdd(std::make_unique<int>(1), 6));
    ASSERT_FALSE(queue.tryAdd(std::make_unique<int>(2), 5));
    ASSERT_TRUE(queue.tryAdd(std::make_unique<int>(3), 4));
    ASSERT_FALSE(queue.tryAdd(std::make_unique<int>(4), 0));
    std::unique_ptr<int> item;
    ASSERT_FALSE(queue.next(item, reject));
    ASSERT_TRUE(queue.next(item, accept));
    ASSERT_EQ(*item, 1);
    ASSERT_TRUE(queue.tryAdd(std::make_unique<int>(5), 6));
    queue.clear();
    ASSERT_TRUE(queue.tryAdd(std::make_unique<int>(6), 10));
    ASSERT_TRUE(queue.next(item, accept));
    ASSERT_EQ(*item, 6);
    ASSERT_FALSE(queue.next(item, accept));
    ASSERT_FALSE(queue.tryAdd(std::make_unique<int>(7), std::numeric_limits<size_t>::max()));
}

TEST(Timer_ImmediateCallbacksAreNotLost)
{
    IO::Timer::AsyncSystemTimer timer;
    std::promise<void> done;
    auto result = done.get_future();
    std::atomic<unsigned> count{0};
    for (unsigned i = 0; i < 500; ++i)
        timer.ScheduleFunctionOnce(0ms, [&]() { if (++count == 500) done.set_value(); });
    bool const completed = result.wait_for(5s) == std::future_status::ready;
    timer.RemoveAllTimersAndStopThread();
    ASSERT_TRUE(completed);
    ASSERT_EQ(count.load(), 500u);
}

TEST(Timer_CancelReleasesCapturesAndSurvivesSchedulerDestruction)
{
    std::shared_ptr<IO::Timer::TimerHandle> handle;
    auto token = std::make_shared<int>(42);
    std::weak_ptr<int> observed = token;
    {
        IO::Timer::AsyncSystemTimer timer;
        handle = timer.ScheduleFunctionOnce(1h, [token]() {});
        token.reset();
        handle->Cancel();
        ASSERT_TRUE(observed.expired());
    }
    handle->Cancel();
}

TEST(Timer_CancelWaitsForRunningCallback)
{
    IO::Timer::AsyncSystemTimer timer;
    std::promise<void> entered, release;
    auto ready = entered.get_future();
    auto gate = release.get_future().share();
    auto handle = timer.ScheduleFunctionOnce(0ms, [&]() { entered.set_value(); gate.wait(); });
    bool const started = ready.wait_for(5s) == std::future_status::ready;
    std::promise<void> cancelEntered;
    auto cancelReady = cancelEntered.get_future();
    auto canceled = std::async(std::launch::async, [&]() { cancelEntered.set_value(); handle->Cancel(); });
    cancelReady.wait();
    bool const waited = canceled.wait_for(20ms) == std::future_status::timeout;
    release.set_value();
    canceled.get();
    timer.RemoveAllTimersAndStopThread();
    ASSERT_TRUE(started);
    ASSERT_TRUE(waited);
}

TEST(Timer_CallbackCanCancelItself)
{
    IO::Timer::AsyncSystemTimer timer;
    std::promise<void> assigned, done;
    auto gate = assigned.get_future().share();
    auto result = done.get_future();
    std::shared_ptr<IO::Timer::TimerHandle> handle;
    handle = timer.ScheduleFunctionOnce(0ms, [&]() { gate.wait(); handle->Cancel(); done.set_value(); });
    assigned.set_value();
    bool const completed = result.wait_for(5s) == std::future_status::ready;
    timer.RemoveAllTimersAndStopThread();
    ASSERT_TRUE(completed);
}

TEST(ThreadPool_RepeatedBatchesCompleteExactlyOnce)
{
    ThreadPool pool("Regression", 4);
    pool.start<>();
    std::atomic<unsigned> completed{0};
    bool ready = true;
    for (unsigned batch = 0; batch < 100; ++batch)
    {
        ThreadPool::workload_t work;
        for (unsigned i = 0; i < 32; ++i) work.emplace_back([&]() { ++completed; });
        auto done = pool.processWorkload(std::move(work));
        if (!done.valid() || done.wait_for(5s) != std::future_status::ready) { ready = false; break; }
        done.get();
    }
    ASSERT_TRUE(ready);
    ASSERT_EQ(completed.load(), 3200u);
}

TEST(ThreadPool_TaskExceptionsCompleteTheBatchAndAllowReuse)
{
    ThreadPool pool("Exceptions", 4, ThreadPool::ClearMode::AT_NEXT_WORKLOAD, ThreadPool::ErrorHandling::LOG);
    pool.start<>();
    ThreadPool::workload_t work;
    for (unsigned i = 0; i < 16; ++i)
        work.emplace_back([]() { throw std::runtime_error("expected regression failure"); });
    auto result = pool.processWorkload(std::move(work));
    bool const ready = result.valid() && result.wait_for(5s) == std::future_status::ready;
    bool threw = false;
    if (ready) { try { result.get(); } catch (std::runtime_error const&) { threw = true; } }
    ASSERT_TRUE(ready);
    ASSERT_TRUE(threw);
    ASSERT_EQ(pool.taskErrors().size(), size_t(16));
    std::atomic<bool> ran{false};
    ThreadPool::workload_t next;
    next.emplace_back([&]() { ran = true; });
    auto reused = pool.processWorkload(std::move(next));
    ASSERT_TRUE(reused.valid());
    reused.get();
    ASSERT_TRUE(ran.load());
    ASSERT_TRUE(pool.taskErrors().empty());
}

TEST(ThreadPool_ShutdownWaitsForActiveWork)
{
    auto pool = std::make_unique<ThreadPool>("Shutdown", 1);
    pool->start<>();
    std::promise<void> entered, release;
    auto started = entered.get_future();
    auto gate = release.get_future().share();
    ThreadPool::workload_t work;
    work.emplace_back([&]() { entered.set_value(); gate.wait(); });
    auto result = pool->processWorkload(std::move(work));
    bool const running = started.wait_for(5s) == std::future_status::ready;
    auto stopped = std::async(std::launch::async, [&]() { pool.reset(); });
    bool const waited = stopped.wait_for(20ms) == std::future_status::timeout;
    release.set_value();
    stopped.get();
    ASSERT_TRUE(running);
    ASSERT_TRUE(waited);
    ASSERT_TRUE(result.wait_for(1s) == std::future_status::ready);
}

TEST(Timer_ShutdownReleasesPendingCallbacks)
{
    IO::Timer::AsyncSystemTimer timer;
    auto token = std::make_shared<int>(1);
    std::weak_ptr<int> observed = token;
    auto handle = timer.ScheduleFunctionOnce(1h, [token]() {});
    token.reset();
    timer.RemoveAllTimersAndStopThread();
    // External handles remain valid, but shutdown must not retain captured resources.
    ASSERT_TRUE(observed.expired());
    handle->Cancel();
    ASSERT_TRUE(timer.ScheduleFunctionOnce(0ms, []() {}) == nullptr);
}

TEST(Transaction_ExceptionRollsBackWithoutReplay)
{
    FakeDatabase db;
    FakeConnection connection(db, db.plan);
    db.plan.throwWrite = true;
    SqlTransaction tx(1);
    tx.DelayExecute(new SqlPlainRequest("write"));
    ASSERT_FALSE(tx.Execute(&connection));
    ASSERT_EQ(db.plan.writes, 1);
    ASSERT_EQ(db.plan.rollbacks, 1);
    ASSERT_EQ(db.plan.commits, 0);
}

TEST(Database_BackgroundWriteFailureIsReportedAndLatched)
{
    DatabaseMysql mysqlLibrary;
    FakeDatabase db;
    ASSERT_TRUE(db.Initialize("localhost;3306;test;test;test", 1, 1));
    db.plan.failWrite = 1;
    db.AllowAsyncTransactions();
    ASSERT_TRUE(db.Execute("injected failing write"));
    auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!db.HasAsyncWriteError() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(1ms);
    bool const latched = db.HasAsyncWriteError();
    bool const rejected = !db.Execute("must not continue writing");
    db.StopServer();
    ASSERT_TRUE(latched);
    ASSERT_TRUE(rejected);
    ASSERT_EQ(db.plan.writes, 1);
}

TEST(Database_CancellationDrainsBothCallbackQueues)
{
    struct Callback : MaNGOS::IQueryCallback
    {
        int& executed;
        int& destroyed;
        std::unique_ptr<QueryResult> result;
        Callback(int& e, int& d) : executed(e), destroyed(d) {}
        ~Callback() override { ++destroyed; }
        void Execute() override { ++executed; }
        void SetResult(std::unique_ptr<QueryResult> r) override { result = std::move(r); }
        std::unique_ptr<QueryResult>& GetResult() override { return result; }
    };
    int executed = 0, destroyed = 0;
    SqlResultQueue queue;
    queue.add(new Callback(executed, destroyed));
    queue._threadUnsafeWaitingQueries.add(new Callback(executed, destroyed));
    queue.numUnsafeQueries = 1;
    queue.CancelAll();
    ASSERT_EQ(executed, 2);
    ASSERT_EQ(destroyed, 2);
    ASSERT_EQ(queue.numUnsafeQueries, 0u);
    queue.CancelAll();
    ASSERT_EQ(executed, 2);
}

TEST(ThreadPool_FailedPreparationDoesNotRunWorkOrCleanup)
{
    ThreadPool pool("Preparation", 1, ThreadPool::ClearMode::AT_NEXT_WORKLOAD, ThreadPool::ErrorHandling::LOG);
    pool.start<>();
    std::atomic<unsigned> ran{0}, cleaned{0};
    ThreadPool::workload_t work;
    work.emplace_back([&]() { ++ran; });
    auto result = pool.processWorkload(std::move(work), []() { throw std::runtime_error("injected preparation failure"); }, [&]() { ++cleaned; });
    bool const ready = result.valid() && result.wait_for(5s) == std::future_status::ready;
    bool threw = false;
    if (ready) { try { result.get(); } catch (std::runtime_error const&) { threw = true; } }
    ASSERT_TRUE(ready);
    ASSERT_TRUE(threw);
    ASSERT_EQ(ran.load(), 0u);
    ASSERT_EQ(cleaned.load(), 0u);
}

TEST(Transaction_DeadlockRetryIsBounded)
{
    FakeDatabase db;
    FakeConnection connection(db, db.plan);
    db.plan.alwaysFail = true;
    db.plan.deadlock = true;
    SqlTransaction tx(1);
    tx.DelayExecute(new SqlPlainRequest("write"));
    ASSERT_FALSE(tx.Execute(&connection));
    ASSERT_EQ(db.plan.begins, 3);
    ASSERT_EQ(db.plan.writes, 3);
    ASSERT_EQ(db.plan.rollbacks, 3);
    ASSERT_EQ(db.plan.commits, 0);
}

TEST(Transaction_FailedRollbackPreventsRetry)
{
    FakeDatabase db;
    FakeConnection connection(db, db.plan);
    db.plan.alwaysFail = true;
    db.plan.deadlock = true;
    db.plan.rollbackOk = false;
    SqlTransaction tx(1);
    tx.DelayExecute(new SqlPlainRequest("write"));
    ASSERT_FALSE(tx.Execute(&connection));
    ASSERT_EQ(db.plan.begins, 1);
    ASSERT_EQ(db.plan.writes, 1);
    ASSERT_EQ(db.plan.rollbacks, 1);
}

TEST(Database_InitializationExceptionsUnwindCreatedConnections)
{
    DatabaseMysql mysqlLibrary;
    FakeDatabase db;
    for (int fail = 0; fail < 5; ++fail)
    {
        db.plan.created = 0;
        db.plan.throwConnection = fail;
        ASSERT_FALSE(db.Initialize("localhost;3306;test;test;test", 1, 3));
        ASSERT_EQ(db.plan.live, 0);
    }
}
