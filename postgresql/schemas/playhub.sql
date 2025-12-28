DROP SCHEMA IF EXISTS playhub CASCADE;

CREATE SCHEMA IF NOT EXISTS playhub;


CREATE TYPE library.game_status AS ENUM (
    'unspecified',  
    'plan',         
    'playing',      
    'completed',    
    'dropped',      
    'waiting'       
);

CREATE TABLE IF NOT EXISTS playhub.library (
    user_id UUID NOT NULL,
    game_id UUID NOT NULL,

    game_status library.game_status NOT NULL DEFAULT 'unspecified',

    created_at TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT NOW()

    PRIMARY KEY (user_id, game_id)

    CONSTRAINT uq_library_game_id UNIQUE (game_id)
);

CREATE INDEX idx_library_entries_user_status ON library.library_entries(user_id, game_status);
