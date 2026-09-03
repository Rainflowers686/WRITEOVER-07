# 20_ERROR_HANDLING_LOGGING (REPAIR PASS v2)

> 修订：`Result<T>` 采用 `std::variant<T, ErrorInfo>` 实现——**支持非
> 默认构造 T**（旧版 raw optional 式实现做不到，是验收报告 M 17.5 项）；
> 异常政策措辞修正：`/EHsc` 不等于 "no exceptions"，真正的规则是**我们的代码
> 零显式 throw/try/catch**（STL 分配失败等仍可能抛出 → 顶层进程策略处理）。
> 实现：`common/result.h`、`common/logging.h`。

## 错误分类（冻结）

| 类 | 例子 | 处理 |
|----|------|------|
| Programmer error | 空指针前提、越界前提 | WO_ASSERT（Debug fail-fast） |
| Recoverable runtime | 文件打不开、设备失败 | `Result<T>` + 明确 fallback |
| 外部数据损坏 | 坏存档/坏内容 | fail-closed：拒载 + 用户可见消息 |
| 用户输入错误 | 未绑定键/非法设置 | 反馈 + 回退默认 |
| 致命系统错误 | 终端初始化失败 | 干净退出 + ConsoleGuard 恢复 + 错误消息 |

## Result<T>（common/result.h）

```cpp
template<typename T> class Result {   // Storage = std::variant<T, ErrorInfo>
    static Result Ok(T value); static Result Err(uint32_t code, std::string msg);
    bool IsOk() const; T& Value(); const ErrorInfo& Error();
};
template<> class Result<void> { ... };  // Ok()/Err(...) 同形
```

- 优点：T 可为不可默认构造类型；ValueOr 兜底；错误永不静默。

## 异常政策（准确表述）

- `/EHsc` 编译；**我们的代码不写 throw/try/catch**。
- 文档层面：把 "/EHsc 视为 no exceptions" 的说法已删除（不正确）；
  真规则 = "zero explicit exception handling in our code；STL 可能抛出的
  分配失败属顶层进程策略，不做 catch 恢复"。
- Forbidden：`catch(...){}`、`try{...}catch(...){return false;}`（check_forbidden
  无 catch 语义机器项，靠审查清单）。

## Logging

- `Logger`（EngineContext.logger 持有，无全局单例）：Debug/Info/Warn/Error/Fatal，
  stderr + 可选 session 文件；线程安全（互斥锁）；模块前缀 `[render]` 等。
- Error/Fatal 恒记录；Debug 关开关。

## 用户可见消息（多语言友好，示例）

损坏存档 / 版本过旧 / 设置无效 / 终端初始化失败 / 无音频自动字幕，均有默认文案。

## 测试锚点

`save.rejects_*` 系列（fail-closed）、`profile`/`settings` 磁盘失败路径。

## 报告友好

**Course Note**: 统一 Result/Error 策略 = Codex 写不出"静默吞错误"。