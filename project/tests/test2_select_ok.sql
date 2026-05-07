-- Test 2: Valid SELECT statements
CREATE TABLE employees (
    emp_id int,
    name varchar(100),
    salary float,
    dept varchar(50)
);

/* Simple SELECT * */
SELECT *
FROM employees;

/* SELECT with WHERE */
SELECT name, salary
FROM employees
WHERE salary >= 50000.0;

/* SELECT with WHERE, GROUP BY, ORDER BY, LIMIT */
SELECT dept, name
FROM employees
WHERE salary > 30000
GROUP BY dept
ORDER BY name
LIMIT 10;

/* SELECT with IN */
SELECT *
FROM employees
WHERE dept IN ('HR', 'IT', 'Finance');

/* SELECT with NOT IN */
SELECT name
FROM employees
WHERE emp_id NOT IN (1, 2, 3);

/* SELECT with NOT condition */
SELECT *
FROM employees
WHERE NOT salary < 20000;

/* SELECT with compound condition */
SELECT name, dept
FROM employees
WHERE salary > 40000 AND dept = 'IT';
