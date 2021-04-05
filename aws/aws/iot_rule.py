import json
from typing import List
import aws_cdk.aws_iot as iot
import aws_cdk.aws_iam as iam
import aws_cdk.aws_logs as logs
from aws_cdk import core as cdk
from aws_cdk.custom_resources import (
    AwsCustomResource,
    AwsCustomResourcePolicy,
    AwsSdkCall,
    PhysicalResourceId,
)


class TimestreamDimension:
    def __init__(self, name: str, value: str) -> None:
        self.dimension = {"name": name, "value": value}


class TimestreamAction(cdk.Construct):
    def __init__(
        self,
        scope: cdk.Construct,
        construct_id: str,
        database_name: str,
        table_name: str,
        dimensions: List[TimestreamDimension],
    ) -> None:
        super().__init__(scope, construct_id)
        self.action_type = "timestream"
        self.database_name = database_name
        self.table_name = table_name
        self.dimensions = dimensions
        self.role = self.create_role(construct_id, database_name, table_name)

    def create_role(self, construct_id: str, database_name: str, table_name: str):
        return iam.Role(
            self,
            id=f"{construct_id}-role",
            assumed_by=iam.ServicePrincipal("iot.amazonaws.com"),
            inline_policies={
                "timestream": iam.PolicyDocument(
                    statements=[
                        iam.PolicyStatement(
                            effect=iam.Effect("ALLOW"),
                            actions=["timestream:WriteRecords"],
                            resources=[
                                f"arn:aws:timestream:eu-west-1:*:database/{self.database_name}/table/{self.table_name}"
                            ],
                        ),
                        iam.PolicyStatement(
                            effect=iam.Effect("ALLOW"),
                            actions=["timestream:DescribeEndpoints"],
                            resources=["*"],
                        ),
                    ]
                )
            },
        )


class TopicRulePayload:
    def __init__(
        self,
        sql: str,
        aws_iot_sql_version="2016-03-23",
        actions: List[TimestreamAction] = [],
    ) -> None:
        self.payload = iot.CfnTopicRule.TopicRulePayloadProperty(
            sql=sql, aws_iot_sql_version=aws_iot_sql_version, actions=actions
        )


class TimestreamRulePayload:
    def __init__(
        self,
        sql: str,
        aws_iot_sql_version="2016-03-23",
        actions: List[TimestreamAction] = [],
    ) -> None:
        self.sql = sql
        self.aws_iot_sql_version = aws_iot_sql_version
        self.actions = actions


class Rule(cdk.Resource):
    def __init__(
        self,
        scope: cdk.Construct,
        construct_id: str,
        rule_name: str,
        topic_rule_payload: TopicRulePayload,
    ) -> None:
        super().__init__(scope, construct_id)
        self.rule = iot.CfnTopicRule(
            self,
            construct_id,
            rule_name=rule_name,
            topic_rule_payload=topic_rule_payload.payload,
        )

class CustomSdkTimestreamRule(cdk.Construct):
    def __init__(
        self,
        scope: cdk.Construct,
        rule_name: str,
        timestream_rule_payload: TimestreamRulePayload,
        log_retention=logs.RetentionDays.ONE_WEEK,
    ) -> None:
        super().__init__(scope, rule_name)

        on_create = self.create_topic_rule(rule_name, timestream_rule_payload)
        on_update = self.replace_topic_rule(rule_name, timestream_rule_payload)
        on_delete = self.delete_topic_rule(rule_name)
        policy = AwsCustomResourcePolicy.from_sdk_calls(
            resources=AwsCustomResourcePolicy.ANY_RESOURCE
        )
        lambda_role = self.get_provisioning_lambda_role(construct_id=rule_name)

        self.resource = AwsCustomResource(
            scope=scope,
            id=f"{rule_name}-AWSCustomResource",
            policy=policy,
            log_retention=log_retention,
            on_create=on_create,
            on_update=on_update,
            on_delete=on_delete,
            resource_type="Custom::AWS-IoT-Rule-Timestream",
            role=lambda_role,
            timeout=None,
        )
        self.resource.grant_principal.add_to_policy(iam.PolicyStatement(actions=["iam:PassRole"], resources=["arn:aws:iam::*"]))


    def get_provisioning_lambda_role(self, construct_id: str):
        return iam.Role(
            scope=self,
            id=f"{construct_id}-LambdaRole",
            assumed_by=iam.ServicePrincipal("lambda.amazonaws.com"),
            managed_policies=[
                iam.ManagedPolicy.from_aws_managed_policy_name(
                    "service-role/AWSLambdaBasicExecutionRole"
                )
            ],
        )

    def topic_rule_payload_to_parameter(
        self, topic_rule_payload: TimestreamRulePayload
    ):
        payload = {
            "sql": topic_rule_payload.sql,
            "ruleDisabled": False,
            "awsIotSqlVersion": topic_rule_payload.aws_iot_sql_version,
            "actions": [],
        }
        for action in topic_rule_payload.actions:
            action_pl = {
                action.action_type: {
                    "roleArn": action.role.role_arn,
                    "tableName": action.table_name,
                    "dimensions": [],
                    "databaseName": action.database_name,
                }
            }
            for dimension in action.dimensions:
                action_pl[action.action_type]["dimensions"].append(dimension.dimension)
            payload["actions"].append(action_pl)
        return payload

    def create_topic_rule(
        self, rule_name: str, topic_rule_payload: TimestreamRulePayload
    ):
        params = {
            "ruleName": rule_name,
            "topicRulePayload": self.topic_rule_payload_to_parameter(
                topic_rule_payload
            ),
        }

        return AwsSdkCall(
            action="createTopicRule",
            service="Iot",
            parameters=params,
            physical_resource_id=PhysicalResourceId.of(rule_name),
        )


    def replace_topic_rule(self, rule_name: str, topic_rule_payload: TopicRulePayload):
        params = {
            "ruleName": rule_name,
            "topicRulePayload": self.topic_rule_payload_to_parameter(
                topic_rule_payload
            ),
        }

        return AwsSdkCall(
            action="replaceTopicRule",
            service="Iot",
            parameters=params,
            physical_resource_id=PhysicalResourceId.of(rule_name),
        )

    def delete_topic_rule(self, rule_name: str):
        params = {
            "ruleName": rule_name,
        }

        return AwsSdkCall(
            action="deleteTopicRule",
            service="Iot",
            parameters=params,
            physical_resource_id=PhysicalResourceId.of(rule_name),
        )
