from aws_cdk import core as cdk
import aws_cdk.aws_timestream as timestream


class IoTStack(cdk.Stack):
    def __init__(self, scope: cdk.Construct, construct_id: str, **kwargs) -> None:
        super().__init__(scope, construct_id, **kwargs)

        # Create Timestream database
        database = timestream.CfnDatabase(
            self, "TimestreamDatabase", database_name="bbqmonitor"
        )

        # Create tables
        connectionTable = timestream.CfnTable(
            self,
            "connection",
            database_name="bbqmonitor",
            table_name="connection",
            retention_properties={"MemoryStoreRetentionPeriodInHours": 1, "MagneticStoreRetentionPeriodInDays": 30},
        )
        connectionTable.add_depends_on(database)
        measurementTable = timestream.CfnTable(
            self,
            "measurements",
            database_name="bbqmonitor",
            table_name="measurements",
            retention_properties={"MemoryStoreRetentionPeriodInHours": 6, "MagneticStoreRetentionPeriodInDays": 365},
        )
        measurementTable.add_depends_on(database)
        debugTable = timestream.CfnTable(
            self,
            "debug",
            database_name="bbqmonitor",
            table_name="debug",
            retention_properties={"MemoryStoreRetentionPeriodInHours": 6, "MagneticStoreRetentionPeriodInDays": 30},
        )
        debugTable.add_depends_on(database)
        
