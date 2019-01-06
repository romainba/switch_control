import pymysql.cursors

DB_NAME = "switch_control"
DB_IP = "192.168.1.114"
DB_USERNAME = "root"
DB_PASSWORD = "toto1234"

def connect():
    return pymysql.connect(
        host=DB_IP,
        user=DB_USERNAME,
        # password=DB_PASSWORD,
        db=DB_NAME,
        charset='utf8mb4',
        cursorclass=pymysql.cursors.DictCursor)


def checkTableExists(dbcon, tablename):
    dbcur = dbcon.cursor()
    dbcur.execute("""
        SELECT COUNT(*)
        FROM information_schema.tables
        WHERE table_name = '{0}'
        """.format(tablename.replace('\'', '\'\'')))
    flag = dbcur.fetchone()[u'COUNT(*)']
    dbcur.close()
    return flag
