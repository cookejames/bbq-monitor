import aws_cdk.aws_timestream as timestream
from aws_cdk import core as cdk


class RetentionProperties:
    def __init__(
        self,
        memory_store_retention_period_in_hours: int,
        magnetic_store_retention_period_in_days: int,
    ) -> None:
        self.memory_store_retention_period_in_hours = (
            memory_store_retention_period_in_hours
        )
        self.magnetic_store_retention_period_in_days = (
            magnetic_store_retention_period_in_days
        )


class Database(cdk.Resource):
    def __init__(
        self, scope: cdk.Construct, construct_id: str, database_name: str
    ) -> None:
        super().__init__(scope, construct_id)
        self.name = database_name
        self.database = timestream.CfnDatabase(
            scope, "database", database_name=database_name
        )

    def create_table(self, table_name: str, retention_properties: RetentionProperties):
        table = timestream.CfnTable(
            self.database,
            table_name,
            database_name=self.database.database_name,
            table_name=table_name,
            retention_properties={
                "MemoryStoreRetentionPeriodInHours": retention_properties.memory_store_retention_period_in_hours,
                "MagneticStoreRetentionPeriodInDays": retention_properties.magnetic_store_retention_period_in_days,
            },
        )
        table.add_depends_on(self.database)
        return table
