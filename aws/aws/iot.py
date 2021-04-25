from aws_cdk import core as cdk
import aws_cdk.aws_iot as iot
import aws_cdk.aws_iam as iam
from timestream import Database, RetentionProperties
from iot_rule import (
    CustomSdkTimestreamRule,
    Rule,
    TimestreamAction,
    TimestreamDimension,
    TopicRulePayload,
    TimestreamRulePayload
)


class IoTStack(cdk.Stack):
    def __init__(self, scope: cdk.Construct, construct_id: str, **kwargs) -> None:
        super().__init__(scope, construct_id, **kwargs)

        # Create Timestream database and table
        database = Database(self, "TimestreamDatabase", "bbqmonitor")
        connection_table = database.create_table(
            "connection", RetentionProperties(6, 30)
        )
        measurement_table = database.create_table(
            "measurements", RetentionProperties(6, 365)
        )
        device_table = database.create_table(
            "device", RetentionProperties(6, 30)
        )
        debug_table = database.create_table("debug", RetentionProperties(6, 30))

        # Create rules
        iam_connection_republish_role = iam.Role(
            self,
            id="iot-rule-action-republish",
            assumed_by=iam.ServicePrincipal("iot.amazonaws.com"),
            inline_policies={
                "republish": iam.PolicyDocument(
                    statements=[
                        iam.PolicyStatement(
                            effect=iam.Effect("ALLOW"),
                            actions=["iot:Publish"],
                            resources=[
                                "arn:aws:iot:eu-west-1:*:topic/$aws/things/*/shadow/name/connection/update"
                            ],
                        )
                    ]
                )
            },
        )

        connection_rule = Rule(
            self,
            "republish_rule",
            "bbq_monitor_connect_republish",
            topic_rule_payload=TopicRulePayload(
                sql="SELECT * FROM 'bbqmonitor/connection/+/updates'",
                actions=[
                    {
                        "republish": iot.CfnTopicRule.RepublishActionProperty(
                            role_arn=iam_connection_republish_role.role_arn,
                            topic="$$aws/things/${topic(3)}/shadow/name/connection/update",
                            qos=1,
                        )
                    }
                ],
            ),
        )

        timestream_connection_rule = CustomSdkTimestreamRule(
            self,
            "bbq_device_connected",
            TimestreamRulePayload(
                sql="SELECT state.reported.* FROM '$aws/things/+/shadow/name/connection/update/accepted'",
                actions=[
                    TimestreamAction(
                        scope=self,
                        construct_id="bbq_device_connected_action",
                        database_name=database.name,
                        table_name=connection_table.table_name,
                        dimensions=[TimestreamDimension("device", "${topic(3)}")],
                    )
                ],
            ),
        )

        timestream_temperature_monitoring_rule = CustomSdkTimestreamRule(
            self,
            "bbq_device_temperature_monitoring",
            TimestreamRulePayload(
                sql="SELECT state.reported.* FROM '$aws/things/+/shadow/name/temperature/update/accepted'",
                actions=[
                    TimestreamAction(
                        scope=self,
                        construct_id="bbq_device_temperature_monitoring_action",
                        database_name=database.name,
                        table_name=measurement_table.table_name,
                        dimensions=[TimestreamDimension("device", "${topic(3)}"), TimestreamDimension("metric", "temperature")],
                    )
                ],
            ),
        )

        timestream_pid_debug_rule = CustomSdkTimestreamRule(
            self,
            "bbq_pid_debug",
            TimestreamRulePayload(
                sql="SELECT pid.* FROM 'bbqmonitor/debug/+/updates'",
                actions=[
                    TimestreamAction(
                        scope=self,
                        construct_id="bbq_pid_debug_action",
                        database_name=database.name,
                        table_name=debug_table.table_name,
                        dimensions=[TimestreamDimension("device", "${topic(3)}"), TimestreamDimension("metric", "pid_output")],
                    )
                ],
            ),
        )

        timestream_device_rule = CustomSdkTimestreamRule(
            self,
            "bbq_device_controlstate_monitoring",
            TimestreamRulePayload(
                sql="SELECT state.reported.* FROM '$aws/things/+/shadow/name/controlstate/update/accepted'",
                actions=[
                    TimestreamAction(
                        scope=self,
                        construct_id="bbq_device_controlstate_monitoring_action",
                        database_name=database.name,
                        table_name=device_table.table_name,
                        dimensions=[TimestreamDimension("device", "${topic(3)}"), TimestreamDimension("metric", "control_state")],
                    )
                ],
            ),
        )

         timestream_device_battery_rule = CustomSdkTimestreamRule(
            self,
            "bbq_device_battery",
            TimestreamRulePayload(
                sql="SELECT state.reported.* FROM '$aws/things/+/shadow/name/battery/update/accepted'",
                actions=[
                    TimestreamAction(
                        scope=self,
                        construct_id="bbq_device_battery_action",
                        database_name=database.name,
                        table_name=device_table.table_name,
                        dimensions=[TimestreamDimension("device", "${topic(3)}"), TimestreamDimension("metric", "battery")],
                    )
                ],
            ),
        )
