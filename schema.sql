CREATE TABLE IF NOT EXISTS users (
    user_id VARCHAR(50) PRIMARY KEY,
    username VARCHAR(100) NOT NULL UNIQUE,
    email VARCHAR(150) NOT NULL UNIQUE,
    role VARCHAR(20) NOT NULL DEFAULT 'STUDENT',
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);
CREATE TABLE IF NOT EXISTS submissions (
    submission_id VARCHAR(50) PRIMARY KEY,
    user_id VARCHAR(50) NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    filename VARCHAR(255) NOT NULL,
    content_text TEXT,
    submitted_at TIMESTAMP NOT NULL DEFAULT NOW(),
    status VARCHAR(20) NOT NULL DEFAULT 'PENDING'
);
CREATE TABLE IF NOT EXISTS fingerprint_hashes (
    hash_id SERIAL PRIMARY KEY,
    submission_id VARCHAR(50)
        REFERENCES submissions(submission_id)
        ON DELETE CASCADE,
    hash_value VARCHAR(256) NOT NULL,
    hash_type VARCHAR(30) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT NOW(),
    UNIQUE(submission_id, hash_value)
);
CREATE TABLE IF NOT EXISTS raw_matches (
    match_id SERIAL PRIMARY KEY,
    batch_id VARCHAR(50) NOT NULL,
    submission_id VARCHAR(50)
        REFERENCES submissions(submission_id)
        ON DELETE CASCADE,
    matched_source_id VARCHAR(50)
        REFERENCES submissions(submission_id)
        ON DELETE CASCADE,
    exact_score DECIMAL(5,4) NOT NULL CHECK (exact_score BETWEEN 0 AND 1),
    structural_score DECIMAL(5,4) NOT NULL CHECK (structural_score BETWEEN 0 AND 1),
    matched_fragment TEXT,
    match_start_index INT,
    match_end_index INT,
    detected_at TIMESTAMP NOT NULL DEFAULT NOW()
);
CREATE TABLE IF NOT EXISTS ranked_results (
    result_id SERIAL PRIMARY KEY,
    submission_id VARCHAR(50)
        REFERENCES submissions(submission_id)
        ON DELETE CASCADE,
    aggregated_score DECIMAL(6,2)
        NOT NULL CHECK (aggregated_score BETWEEN 0 AND 100),
    plagiarism_label VARCHAR(10) NOT NULL,
    rank_position INT NOT NULL,
    batch_id VARCHAR(50) NOT NULL,
    processed_at TIMESTAMP NOT NULL DEFAULT NOW(),
    UNIQUE(submission_id, batch_id)
);
CREATE INDEX idx_raw_matches_submission
ON raw_matches(submission_id);

CREATE INDEX idx_raw_matches_batch
ON raw_matches(batch_id);

CREATE INDEX idx_ranked_results_batch
ON ranked_results(batch_id);

CREATE INDEX idx_fingerprint_hashes_submission
ON fingerprint_hashes(submission_id);
