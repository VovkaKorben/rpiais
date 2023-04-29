SELECT id,
    description,
    -- obsolete,
    grouped
FROM sentences
WHERE handled = 1
ORDER BY id ASC;