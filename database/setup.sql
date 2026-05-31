-- ============================================================
-- ContentGuard – Complete Database Setup
-- Run this ONCE in Neon SQL Editor
-- ============================================================

-- Module 1: Users Table
CREATE TABLE IF NOT EXISTS Users (
    userId        SERIAL PRIMARY KEY,
    name          VARCHAR(100) NOT NULL,
    email         VARCHAR(100) NOT NULL UNIQUE,
    role          VARCHAR(20)  NOT NULL CHECK (role IN ('STUDENT', 'INSTRUCTOR'))
);

-- Module 1: Submissions Table
CREATE TABLE IF NOT EXISTS Submissions (
    doc_id        SERIAL PRIMARY KEY,
    userId        INT          NOT NULL REFERENCES Users(userId) ON DELETE CASCADE,
    fileName      VARCHAR(200) NOT NULL,
    content       TEXT         NOT NULL,
    fileType      VARCHAR(10)  NOT NULL CHECK (fileType IN ('TEXT', 'CODE')),
    status        VARCHAR(20)  NOT NULL DEFAULT 'PENDING'
                               CHECK (status IN ('PENDING','PROCESSING','DONE','ERROR')),
    submittedAt   TIMESTAMP    NOT NULL DEFAULT NOW(),
    fileSizeBytes INT          NOT NULL
);

-- Module 2: Preprocessed Documents Table
CREATE TABLE IF NOT EXISTS PreprocessedDocs (
    preprocessId      SERIAL PRIMARY KEY,
    doc_id            INT         NOT NULL REFERENCES Submissions(doc_id) ON DELETE CASCADE,
    normalizedContent TEXT        NOT NULL,
    tokenCount        INT         NOT NULL,
    charCount         INT         NOT NULL,
    detectedType      VARCHAR(10) NOT NULL CHECK (detectedType IN ('TEXT','CODE')),
    processedAt       TIMESTAMP   NOT NULL DEFAULT NOW()
);

-- Indexes for faster queries
CREATE INDEX IF NOT EXISTS idx_submissions_userId ON Submissions(userId);
CREATE INDEX IF NOT EXISTS idx_submissions_status ON Submissions(status);
CREATE INDEX IF NOT EXISTS idx_preprocessed_doc_id ON PreprocessedDocs(doc_id);

-- Module 3: Fingerprint index (also auto-created by backend on first insert)
CREATE TABLE IF NOT EXISTS fingerprints (
    id         SERIAL PRIMARY KEY,
    doc_id     INT    NOT NULL REFERENCES Submissions(doc_id) ON DELETE CASCADE,
    hash_value BIGINT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_fingerprints_hash ON fingerprints(hash_value);
CREATE INDEX IF NOT EXISTS idx_fingerprints_doc  ON fingerprints(doc_id);

-- Module 4/5: Persisted scan reports (JSON + scores)
CREATE TABLE IF NOT EXISTS scan_results (
    doc_id          INT PRIMARY KEY REFERENCES Submissions(doc_id) ON DELETE CASCADE,
    score           DOUBLE PRECISION NOT NULL DEFAULT 0,
    label           VARCHAR(16),
    classification  VARCHAR(16),
    rank_position   INT DEFAULT 0,
    batch_id        VARCHAR(64),
    report_json     TEXT NOT NULL,
    updated_at      TIMESTAMP NOT NULL DEFAULT NOW()
);

-- Module 5: Ranked batch results (optional persistence)
CREATE TABLE IF NOT EXISTS ranked_results (
    id                SERIAL PRIMARY KEY,
    submission_id     VARCHAR(32) NOT NULL,
    aggregated_score  DOUBLE PRECISION NOT NULL,
    plagiarism_label  VARCHAR(16) NOT NULL,
    rank_position     INT NOT NULL,
    batch_id          VARCHAR(64) NOT NULL,
    created_at        TIMESTAMP NOT NULL DEFAULT NOW()
);

SELECT 'ContentGuard tables created successfully!' AS result;
