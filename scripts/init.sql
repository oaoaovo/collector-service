CREATE TABLE IF NOT EXISTS Device (
    Id INTEGER PRIMARY KEY AUTOINCREMENT,
    Name TEXT NOT NULL UNIQUE,
    IpAddr TEXT,
    Port INTEGER,
    SampleInterval INTEGER,
    Status INTEGER NOT NULL DEFAULT 1,
    Model TEXT,
    ModelVersion TEXT,
    DriverPath TEXT
);

CREATE TABLE IF NOT EXISTS DataPoint (
    Id INTEGER PRIMARY KEY AUTOINCREMENT,
    Did INTEGER,
    DName TEXT,
    NamePath TEXT,
    ItemPath TEXT,
    ItemName TEXT,
    ItemDescription TEXT,
    UNIQUE(DName, NamePath)
);

CREATE TABLE IF NOT EXISTS Resources (
    Id INTEGER PRIMARY KEY AUTOINCREMENT,
    DeviceName TEXT,
    DataItemNamePath TEXT,
    Data TEXT,
    DevTime REAL,
    SvrTime REAL,
    DataItemPath TEXT,
    DataItemName TEXT,
    DataItemDescription TEXT
);
